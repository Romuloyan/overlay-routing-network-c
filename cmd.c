#include "cmd.h"



/* -------------------------------------------------------------------------
 * FUNÇÃO: nodes_querry
 * Descrição: Envia um pedido UDP ao servidor para obter a lista de nós 
 * ativos numa rede específica.
 * Argumentos: 
 * - node: Estado global do nó.
 * - net_to_check: String com o identificador da rede a consultar.
 * Retorno: void
 * ------------------------------------------------------------------------- */
void nodes_querry(NodeState *node, char *net_to_check) {
    if (!parse_network_id(net_to_check)) {
        printf("[Erro] A rede deve ter exatamente 3 dígitos.\n");
        return;
    }
    char message[128];
    node->last_udp_tid = gerar_tid();
    snprintf(message, sizeof(message), "NODES %03d 0 %s\n", node->last_udp_tid, net_to_check);
    enviar_mensagem_udp(node->net.udp_fd, node->net.udp_res, message);
}
/* -------------------------------------------------------------------------
 * FUNÇÃO: join
 * Descrição: Inicia o processo de registo formal do nó no servidor de nós (UDP).
 * Argumentos:
 * - node: Estado global do nó.
 * Retorno: void
 * Notas: Verifica a validade do TID e das configurações de rede antes do envio.
 * ------------------------------------------------------------------------- */
void join(NodeState *node) {
    char message[128];
    node->last_udp_tid = gerar_tid();


    if(node->last_udp_tid == -1) {
        printf("[Erro] Não foi possível gerar um TID válido para o registo.\n");
        return;
    }
    if (!parse_node_id(node->self.id, NULL) || !parse_network_id(node->self.net)) {
        printf("[Erro] ID ou rede não especificados. Use 'join <net> <id>'.\n");
        return;
    }
    if(strlen(node->self.ip) == 0 || strlen(node->self.port) == 0) {
        printf("[Erro] IP ou porto do nó não configurados. Verifique a configuração de rede.\n");
        return;
    }
    if(node->net.udp_fd == -1 || node->net.udp_res == NULL) {
        printf("[Erro] Conexão UDP não configurada. Verifique a inicialização da rede.\n");
        return;
    }
    if(node->num_neighbors > 0) {
        printf("[Aviso] O nó tem vizinhos na tabela. Certifique-se de que a rede está limpa antes de registar um novo ID.\n");
    }
    
    snprintf(message, sizeof(message), "REG %03d 0 %s %s %s %s\n", 
             node->last_udp_tid, node->self.net, node->self.id, node->self.ip, node->self.port);

    
    enviar_mensagem_udp(node->net.udp_fd, node->net.udp_res, message);


}
/* -------------------------------------------------------------------------
 * FUNÇÃO: direct_join
 * Descrição: Ativa o nó localmente sem efetuar registo no servidor UDP.
 * Argumentos:
 * - node: Estado global do nó.
 * Retorno: void
 * Notas: Define a distância para o próprio nó como 0 e o estado is_registered como 2.
 * ------------------------------------------------------------------------- */
void direct_join(NodeState *node) {
    
    // 1. Verificações de segurança
    if(strlen(node->self.id) == 0 || strlen(node->self.net) == 0) {
        printf("[Erro] ID ou rede não especificados. Use 'dj <net> <id>'.\n");
        return;
    }
    
    // 2. Ativação Stealth
   
    node->is_registered = 2; 
    
    // 3. Ponto de Ignição do Encaminhamento
    int my_id_int;
    if (!parse_node_id(node->self.id, &my_id_int)) {
        printf("[Erro] ID do nó inválido.\n");
        return;
    }
    node->routing_table.dist[my_id_int] = 0;
    node->routing_table.succ[my_id_int] = my_id_int;
    node->routing_table.state[my_id_int] = 0; // Garante o estado de expedição

    printf("-> Nó %02d entrou na rede %s em modo Direct Join.\n", my_id_int, node->self.net);
}
/* -------------------------------------------------------------------------
 * FUNÇÃO: direct_add_edge
 * Descrição: Cria uma ligação TCP direta a um IP/Porto sem consultar o servidor.
 * Argumentos:
 * - node: Estado global.
 * - id_vizinho, ip_vizinho, port_vizinho: Dados do destino.
 * Retorno: void
 * Notas: Implementa um timeout de 3 segundos no connect para evitar bloqueios.
 * ------------------------------------------------------------------------- */
void direct_add_edge(NodeState *node, char *id_vizinho, char *ip_vizinho, char *port_vizinho) {
    if(node->num_neighbors >= MAX_NEIGHBORS) {
        printf("[Aviso] Tabela de vizinhos cheia.\n");
        return;
    }
    if(strcmp(id_vizinho, node->self.id) == 0) {
        printf("[Aviso] Não é possível adicionar o próprio nó como vizinho.\n");
        return;
    }
    
    // --- VERIFICAÇÃO EXTRA DE SEGURANÇA (ID) ---
    if (!parse_node_id(id_vizinho, NULL)) {
        printf("[Erro] O ID do vizinho deve ter exatamente 2 dígitos.\n");
        return;
    }
    if (strlen(ip_vizinho) >= sizeof(node->neighbors[0].ip) ||
        strlen(port_vizinho) >= sizeof(node->neighbors[0].port)) {
        printf("[Erro] IP ou porto do vizinho demasiado longo.\n");
        return;
    }

    int sock_fd;
    struct addrinfo *res;

    if (configurar_conexao_tcp_cliente(ip_vizinho, port_vizinho, &sock_fd, &res) != 0) {
        return;
    }
    

    struct timeval timeout;
    timeout.tv_sec = 3;  // 3 segundos de limite de paciência
    timeout.tv_usec = 0;
    setsockopt(sock_fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
   

    if (connect(sock_fd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("[Erro] Falha na ligação direta");
        close(sock_fd);
        freeaddrinfo(res);
        return;
    }
    
    // 2. Ligação bem sucedida! Guardamos os dados do vizinho
    int index = node->num_neighbors;
    snprintf(node->neighbors[index].id, sizeof(node->neighbors[index].id), "%s", id_vizinho);
    snprintf(node->neighbors[index].ip, sizeof(node->neighbors[index].ip), "%s", ip_vizinho);
    snprintf(node->neighbors[index].port, sizeof(node->neighbors[index].port), "%s", port_vizinho);
    node->neighbors[index].fd = sock_fd;
    node->num_neighbors++;

    char greeting[128];
    // Envia o cumprimento NEIGHBOR para o novo vizinho
    snprintf(greeting, sizeof(greeting), "NEIGHBOR %s\n", node->self.id);
    enviar_mensagem_tcp(sock_fd, greeting); 
    
    if(node->monitor == 1) {
        printf("[Monitor] Mensagem TCP enviada para %s: %s", id_vizinho, greeting);
    }
    
    partilhar_tabela_com_vizinho(node, sock_fd); 

    freeaddrinfo(res);
    printf("[Sistema] Conexão TCP direta estabelecida com o vizinho %s.\n", id_vizinho);
}
/* -------------------------------------------------------------------------
 * FUNÇÃO: leave
 * Descrição: Encerra a participação do nó na rede.
 * Argumentos:
 * - node: Estado global.
 * Retorno: void
 * Notas: Envia UNREG ao servidor se necessário e fecha todos os sockets ativos.
 * ------------------------------------------------------------------------- */
void leave(NodeState *node) {
    if(!node->is_registered) {
        printf("[Aviso] O nó não está registado. Não é possível sair.\n");
        return;
    }
    if (node->is_registered == 1) {
        char message[128];
        node->last_udp_tid = gerar_tid();
        snprintf(message, sizeof(message), "REG %03d 3 %s %s\n",
                 node->last_udp_tid, node->self.net, node->self.id);
        enviar_mensagem_udp(node->net.udp_fd, node->net.udp_res, message);
    }
    for (int i = 0; i < node->num_neighbors; i++) {
        close(node->neighbors[i].fd);
        if(node->monitor == 1) {
            printf("[Monitor] Conexão TCP com vizinho %s (fd: %d) fechada.\n", node->neighbors[i].id, node->neighbors[i].fd);
        }
    }
    node->num_neighbors = 0; 

    
    init_routing_table(node, MAX_NODES);
    node->is_registered = 0;
    if(node->monitor == 1) {
        printf("-> Nó removido da rede com sucesso.\n");
    }
}
/* -------------------------------------------------------------------------
 * FUNÇÃO: help
 * Descrição: Exibe o manual de comandos disponíveis no terminal.
 * ------------------------------------------------------------------------- */
void help() {
    printf("\n--- MANUAIS DE VOO (COMANDOS) ---\n");
    printf("  join <net> <id> (j)  - Regista o nó na rede\n");
    printf("  direct join <net> <id> (dj) - Ativa o nó sem falar com o servidor\n");
    printf("  nodes <net> (n)     - Lista nós de uma rede\n");
    printf("  leave (l)            - Desregista o nó da rede\n");
    printf("  show nodes <net> (n) - Lista nós de uma rede\n");
    printf("  add edge <id> (ae)   - Conecta a um vizinho\n");
    printf("  remove edge <id> (re) - Desconecta de um vizinho\n");
    printf("  show neighbors (sg)  - Mostra a tabela de ligações\n");
    printf("  help (h)             - Mostra este menu\n");
    printf("  exit (x)             - Desliga a nave\n");
    printf("  message <dest> <msg> (m) - Envia mensagem para outro nó\n");
    printf("  start monitor (sm)   - Liga monitor de encaminhamento\n");
    printf("  end monitor (em)     - Desliga monitor de encaminhamento\n");
    printf("---------------------------------\n\n");
}
/* -------------------------------------------------------------------------
 * FUNÇÃO: tratar_mensagem_udp
 * Descrição: Parser central para as mensagens recebidas do servidor UDP.
 * Argumentos:
 * - buffer: Conteúdo bruto da mensagem.
 * - node: Estado global.
 * Retorno: 1 se processada com sucesso, 0 caso contrário.
 * ------------------------------------------------------------------------- */
int tratar_mensagem_udp(char *buffer, NodeState *node) {
    char type[16] = "";
    int rcv_tid, rcv_op;
    char net[4] = "";
    // Extração básica: Tipo, TID e OP (presentes em todas as mensagens v2)
    int header_fields = sscanf(buffer, "%15s %d %d %3s", type, &rcv_tid, &rcv_op, net);
    if (header_fields < 3) {
        printf("[Erro] Mensagem mal formatada: %s\n", buffer);
        return 0;
    }
    // 2. Verificação do TID (Transaction ID) 
    if (rcv_tid != node->last_udp_tid) {
        printf("[Aviso] TID recebido (%d) não condiz com o esperado (%d). Ignorado.\n", 
               rcv_tid, node->last_udp_tid);
        return 0;
    }



    if (strcmp(type, "REG") == 0) {
        
        if (rcv_op == 1) {
            node->is_registered = 1;

        }
        else if (rcv_op == 2) printf("-> Erro: Base de dados CHEIA.\n");
        else if (rcv_op == 4) {
            node->is_registered = 0;
            node->num_neighbors = 0; 
            node->self.id[0] = '\0'; 
        }
        else printf("-> Código REG desconhecido: %d\n", rcv_op);
    } 
    else if (strcmp(type, "NODES") == 0) {
        if (header_fields < 4 || !parse_network_id(net)) {
            printf("[Erro] Resposta NODES mal formatada: %s\n", buffer);
            return 0;
        }
        // op=1 é a lista de IDs 
        if (rcv_op == 1) printf("-> Nós na rede %s: %s\n", net, buffer + strlen(type) + strlen(net) + 1);
        else if (rcv_op == 2) printf("-> Erro: Rede %s não encontrada.\n", net);
    }
    else if (strcmp(type, "CONTACT") == 0) {
        if (header_fields < 4 || !parse_network_id(net)) {
            printf("[Erro] Resposta CONTACT mal formatada: %s\n", buffer);
            return 0;
        }
        
        
        if (rcv_op == 1){
         
            char rcv_id[4], rcv_ip[32], rcv_port[8];
        
            // 1. Extraímos os dados para variáveis locais em vez de escrever logo na tabela
            if (sscanf(buffer, "%*s %*d %*d %*s %3s %31s %7s", rcv_id, rcv_ip, rcv_port) == 3 &&
                parse_node_id(rcv_id, NULL)) {
                
                int ja_existe = 0;
                // 2. Verificamos se este ID já existe na tabela (foi um vizinho que se ligou a nós)
                for (int i = 0; i < node->num_neighbors; i++) {
                    if (strcmp(node->neighbors[i].id, rcv_id) == 0) {
                        // Se já existe, apenas atualizamos o IP e Porto oficiais vindos do servidor
                        snprintf(node->neighbors[i].ip, sizeof(node->neighbors[i].ip), "%s", rcv_ip);
                        snprintf(node->neighbors[i].port, sizeof(node->neighbors[i].port), "%s", rcv_port);
                        //printf("-> Porto oficial do vizinho %s atualizado para: %s\n", rcv_id, rcv_port);
                        ja_existe = 1;
                        break;
                    }
                }
                    if (!ja_existe) {
                        if(node->num_neighbors < MAX_NEIGHBORS) {
                    
                        Neighbor *proximo_vizinho = &node->neighbors[node->num_neighbors];
                        snprintf(proximo_vizinho->id, sizeof(proximo_vizinho->id), "%s", rcv_id);
                        snprintf(proximo_vizinho->ip, sizeof(proximo_vizinho->ip), "%s", rcv_ip);
                        snprintf(proximo_vizinho->port, sizeof(proximo_vizinho->port), "%s", rcv_port);
                    printf("-> Contacto recebido: Nó %s está em %s:%s\n", proximo_vizinho->id, proximo_vizinho->ip, proximo_vizinho->port);
                    if(neighbor_edge_create(node)==1){
                        node->num_neighbors++; // Incrementa o número de vizinhos na tabela
                    } // Aqui você pode implementar a lógica para criar uma conexão TCP com o vizinho e adicioná-lo à tabela de vizinhos.
                    else {
                        memset(proximo_vizinho, 0, sizeof(Neighbor)); // Limpa a entrada do vizinho em caso de falha
                        printf("[Erro] Falha ao criar conexão com o vizinho %s. Vizinho removido da tabela.\n", proximo_vizinho->id);
                    }
                        }
                        else {
                            printf("[Aviso] tabela de vizinhos cheia. Não é possível adicionar o vizinho %s.\n", rcv_id);
                        }
                    }
            }
                else {
                    printf("[Erro] Resposta CONTACT mal formatada: %s\n", buffer);
                }

            
        }else if (rcv_op == 2) printf("-> Nó não registado na rede.\n");

            
        
        
    }
    return 1; // Mensagem válida e processada
}
/* -------------------------------------------------------------------------
 * FUNÇÃO: show_neighbors
 * Descrição: Apresenta a tabela de vizinhos TCP no terminal.
 * ------------------------------------------------------------------------- */
void show_neighbors(NodeState *node) {
    printf("\n==========================================\n");
    printf("        TABELA DE CONEXÕES ATIVAS         \n");
    printf("==========================================\n");
    
    if (node->num_neighbors == 0) {
        printf("Nenhum vizinho conectado de momento.\n");
    } else {
        printf("%-5s | %-15s | %-6s | %-4s\n", "ID", "IP", "PORTO", "FD");
        printf("------------------------------------------\n");
        
        for (int i = 0; i < node->num_neighbors; i++) {
            printf("%-5s | %-15s | %-6s | %-4d\n", 
                   node->neighbors[i].id, 
                   node->neighbors[i].ip, 
                   node->neighbors[i].port,
                   node->neighbors[i].fd);
        }
    }
    printf("------------------------------------------\n\n");
}
/* -------------------------------------------------------------------------
 * FUNÇÃO: add_edge
 * Descrição: Solicita ao servidor UDP o contacto de um nó para abrir ligação TCP.
 * Argumentos:
 * - node: Estado global.
 * - id: ID do nó pretendido.
 * ------------------------------------------------------------------------- */
void add_edge(NodeState *node, const char *id) {
    if(node->num_neighbors >= MAX_NEIGHBORS) {
        printf("[Aviso] Tabela de vizinhos cheia. Não é possível adicionar mais vizinhos.\n");
        return;
    }
    
    if (!parse_node_id(id, NULL)) {
        printf("[Erro] O ID do vizinho deve ter exatamente 2 dígitos.\n");
        return;
    }
    if(strcmp(id, node->self.id) == 0) {
        printf("[Aviso] Não é possível adicionar o próprio nó como vizinho.\n");
        return;
    }
    for (int i = 0; i < node->num_neighbors; i++) {
        if (strcmp(node->neighbors[i].id, id) == 0) {
            printf("[Aviso] Já existe uma ligação com o vizinho %s.\n", id);
            return;
        }
    }
    char message[128];
    node->last_udp_tid = gerar_tid();
    /*Pedido de informação sobre o vizinho*/
    snprintf(message, sizeof(message), "CONTACT %03d 0 %s %s\n", 
             node->last_udp_tid, node->self.net, id);
    enviar_mensagem_udp(node->net.udp_fd, node->net.udp_res, message);

}
/* -------------------------------------------------------------------------
 * FUNÇÃO: remover_vizinho
 * Descrição: Remove um vizinho, aciona coordenação e encerra o socket.
 * Argumentos:
 * - node: Estado global.
 * - id_alvo: ID do nó a remover.
 * ------------------------------------------------------------------------- */
void remover_vizinho(NodeState *node, char *id_alvo) {
    int i, j;
    int encontrado = 0;
    int id_valido = parse_node_id(id_alvo, NULL);

    for (i = 0; i < node->num_neighbors; i++) {
        if (strcmp(node->neighbors[i].id, id_alvo) == 0) {
            // 1. Fechar o socket TCP
            // ---> LIGAÇÃO 1: Acionar o alarme ANTES de apagar o socket! <---
            if (id_valido) {
                coordenation_sender(node, i); // Envia o alarme de coordenação para os vizinhos restantes antes de fechar a conexão
            }

            // 1. Fechar o socket TCP
            close(node->neighbors[i].fd);
            if(node->monitor == 1) {
                printf("[Sistema] Conexão com o nó %s (fd: %d) encerrada.\n", 
                       id_alvo, node->neighbors[i].fd);
            }
          

            // 2. Reorganizar o array (Shift para a esquerda)
            for (j = i; j < node->num_neighbors - 1; j++) {
                node->neighbors[j] = node->neighbors[j + 1];
            }

            // 3. Limpar o último slot que ficou duplicado e decrementar
            memset(&node->neighbors[node->num_neighbors - 1], 0, sizeof(Neighbor));
            node->num_neighbors--;
            
            encontrado = 1;
            break; 
        }
    }

    if (!encontrado) {
        printf("[Aviso] Nó %s não encontrado na tabela de vizinhos.\n", id_alvo);
    }
}
/* -------------------------------------------------------------------------
 * FUNÇÃO: announce
 * Descrição: Difunde a presença do próprio nó (distância 0) aos vizinhos.
 * ------------------------------------------------------------------------- */
void announce(NodeState *node) {
    char message[128];
    int my_id_int;
    if (!parse_node_id(node->self.id, &my_id_int)) {
        printf("[Erro] ID do nó inválido.\n");
        return;
    }
    node->routing_table.dist[my_id_int] = 0;
    node->routing_table.succ[my_id_int] = my_id_int; // We are our own successor
    for(int i=0; i<node->num_neighbors; i++){
        if(node->neighbors[i].fd != -1) {
            snprintf(message, sizeof(message), "ROUTE %s 0\n",
            node->self.id);
            enviar_mensagem_tcp(node->neighbors[i].fd, message);
                if(node->monitor == 1) {
                    printf("[Monitor] Mensagem TCP enviada para %s: %s", node->neighbors[i].id, message);
                }
        }
    }
    
}
/* -------------------------------------------------------------------------
 * FUNÇÃO: show_routing_table
 * Descrição: Exibe o estado do encaminhamento para um destino específico.
 * ------------------------------------------------------------------------- */
void show_routing_table(NodeState *node, char *dest_str) {
    int dest;

    // Proteção contra IDs fora do limite
    if (!parse_node_id(dest_str, &dest)) {
        printf("[Erro] Destino inválido. O ID deve estar entre 00 e 99.\n");
        return;
    }

    int state = node->routing_table.state[dest];
    int dist = node->routing_table.dist[dest];
    int succ = node->routing_table.succ[dest];

    printf("\n=== ROTA PARA O NÓ %02d ===\n", dest);
    
    if (state == 0) {
        printf("Estado: Expedição (0)\n");
        
        if (dist == INF) { // 999 representa o nosso infinito (INF)
            printf("Distância: Inalcançável (Infinito)\n");
            printf("Vizinho de expedição: Nenhum\n");
        } else {
            printf("Distância: %d salto(s)\n", dist);
            printf("Vizinho de expedição: %02d\n", succ);
        }
    } else if (state == 1) {
        printf("Estado: Coordenação (1)\n");
        printf("A aguardar estabilização da rede...\n");
    }
    printf("===========================\n\n");
}
/* -------------------------------------------------------------------------
 * FUNÇÃO: enviar_chat
 * -------------------------------------------------------------------------
 * Descrição: Encapsula e envia uma mensagem de texto para um destino remoto
 * através do próximo salto (sucessor) indicado pela tabela de encaminhamento.
 * Argumentos: 
 * - node: Estado global do nó.
 * - dest_str: String com o ID do nó destino.
 * - msg_texto: Conteúdo da mensagem a transmitir.
 * Retorno: void
 * Detalhes: Verifica se a rota está ativa (estado de expedição) antes de 
 * tentar o envio.
 * ------------------------------------------------------------------------- */
void enviar_chat(NodeState *node, char *dest_str, char *msg_texto) {
    int dest_int;
    if (!parse_node_id(dest_str, &dest_int)) {
        printf("[Erro] Destino inválido. O ID deve estar entre 00 e 99.\n");
        return;
    }
    int succ_id = node->routing_table.succ[dest_int];

    if (succ_id != -1 && node->routing_table.state[dest_int] == 0) {
        char pacote[256];
        snprintf(pacote, sizeof(pacote), "CHAT %s %s %s\n", node->self.id, dest_str, msg_texto);

        int enviou = 0;
        for (int i = 0; i < node->num_neighbors; i++) {
            int neighbor_id;
            if (parse_node_id(node->neighbors[i].id, &neighbor_id) && neighbor_id == succ_id) {
                enviar_mensagem_tcp(node->neighbors[i].fd, pacote);
                printf("-> Mensagem enviada para %s via vizinho %02d.\n", dest_str, succ_id);
                enviou = 1;
                break;
            }
        }
        if (!enviou) printf("[Erro Crítico] Vizinho %02d não encontrado nos sockets.\n", succ_id);
    } else {
        printf("[Erro] Impossível enviar. Rota para %s não existe ou está em quarentena.\n", dest_str);
    }
}

