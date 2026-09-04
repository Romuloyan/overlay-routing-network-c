#include "utils.h"

/* -------------------------------------------------------------------------
 * FUNÇÃO: processar_argumentos
 * Descrição: Faz o parsing dos argumentos da linha de comandos (IP e Porto)
 * e configura os endereços padrão do Servidor de Nós, caso não sejam fornecidos.
 * Argumentos: 
 * - argc/argv: Argumentos do main.
 * - node: Estrutura de estado do nó a preencher.
 * - regInfo: Informação do servidor de registo.
 * Retorno: void
 * ------------------------------------------------------------------------- */
void processar_argumentos(int argc, char *argv[], NodeState *node, RegServerInfo *regInfo) {
    if(argc < 3) {
        fprintf(stderr, "Uso: %s <IP> <TCPport> [regIP] [regPort]\n", argv[0]);
        exit(1);
    }

    regInfo->regIP = "193.136.138.142"; 
    regInfo->regPort = "59000";         

    if (argc >= 4) regInfo->regIP = argv[3]; 
    if (argc >= 5) regInfo->regPort = argv[4]; 


    strncpy(node->self.ip, argv[1], sizeof(node->self.ip) - 1); 
    strncpy(node->self.port, argv[2], sizeof(node->self.port) - 1);
    
    // Garantir que o ID começa vazio até ao 'join'
    node->self.id[0] = '\0'; 
}
/* -------------------------------------------------------------------------
 * FUNÇÃO: teclado
 * Descrição: Interface de linha de comandos (CLI). Lê a entrada do utilizador,
 * interpreta o comando e invoca a lógica correspondente (join, leave, chat, etc).
 * Gere também os formatos curtos (ex: 'j') e longos (ex: 'join').
 * Argumentos: 
 * - node: Estado global do nó.
 * - regInfo: Estrutura com IP/Porto do servidor de registo.
 * Retorno: 1 se o utilizador desejar sair (comando exit), 0 caso contrário.
 * ------------------------------------------------------------------------- */
int teclado( NodeState *node, RegServerInfo regInfo) {
            char buffer[128];// Buffer para ler a linha de comando do utilizador
            
            if(fgets(buffer, sizeof(buffer), stdin) != NULL){ // Lê uma linha de comando do utilizador
                char cmd[16], id_temp[128];// Buffer para extrair o comando da linha de entrada
                char net_temp[128]; // Buffer para extrair o ID temporário da linha de entrada tinha 4 mas aumentei para 16 por causa do show nodes, onde isto armazena o nodes, para acomodar melhor o nome da rede
                int n = sscanf(buffer, "%15s %127s %127s", cmd, net_temp, id_temp); // Extrai o comando e o ID (se fornecido) da linha de entrada
                
                
    /* --- COMANDO: JOIN --- */
               
    if (strcmp(cmd, "join") == 0 || strcmp(cmd, "j") == 0) {
        if (n == 3) {
            
            if (node->is_registered) { // previne alterar o ID ou a rede sem antes sair da rede atual.
                printf("[Aviso] O nó já está registado. Use 'leave' antes de tentar registar um novo ID.\n");
            }
            else {
                
                if (strlen(net_temp) != 3 || strlen(id_temp) != 2) {
                    printf("[Erro] Formato inválido. A rede deve ter 3 dígitos (ex: 042) e o ID 2 dígitos (ex: 01).\n");
                }
                else {
                    strcpy(node->self.id, id_temp); // Capturamos o ID
                    strcpy(node->self.net, net_temp); // Capturamos a rede
                    join(node);
                }
            }
            
        } 
       
        else {
            printf("[Erro] Uso correto: join (j) <net> <id>\n");
        }
    }
    /* --- COMANDO: SHOW NODES --- */
                else if(((strcmp(cmd, "show")  ==0) && ( strcmp(net_temp,"nodes") == 0)) || (strcmp(cmd, "n"))==0){
                    if (n == 2 && strcmp(cmd, "n") == 0) {
                        // Usamos o net_temp que o sscanf já capturou!
                        nodes_querry(node, net_temp); 
                    }else if(n == 3 && strcmp(cmd, "show") == 0){
                        nodes_querry(node, id_temp);
                    } 
                    else {
                        printf("Erro: Uso correto: nodes <net>\n");
                    }
                }
    /* --- COMANDO: LEAVE --- */
                else if((strcmp(cmd, "leave") == 0 || strcmp(cmd, "l") == 0) && n == 1) {
                    leave(node);
                }
    /* --- COMANDO: EXIT --- */
                else if((strcmp(cmd, "exit") == 0 || strcmp(cmd, "x") == 0) && n == 1) {
                    
                    limpar_conexao(node); // Limpa os recursos antes de sair
                    return 1; 
                }
    /* --- COMANDO: ADD EDGE --- */
                else if (((strcmp(cmd, "add") == 0) && (strcmp(net_temp, "edge") == 0)) || (strcmp(cmd, "ae") == 0)) {
                   
                    
                    if (n == 2 && strcmp(cmd, "ae") == 0) {
                        add_edge(node, net_temp);
                    }
                    
                    else if (n == 3 && strcmp(cmd, "add") == 0) {
                        add_edge(node, id_temp);
                    }
                  
                    else {
                        printf("[Erro] Uso correto: ae <id> ou add edge <id>\n");
                    }
                }
    /* --- COMANDO: REMOVE EDGE --- */
                else if (((strcmp(cmd, "remove") == 0) && (strcmp(net_temp, "edge") == 0)) || (strcmp(cmd, "re") == 0) || (strcmp(cmd, "re") == 0)) {
                    
                    
                    if ((strcmp(cmd, "re") == 0 ) && n == 2) {
                        remover_vizinho(node, net_temp);
                    }
                    
                    else if (strcmp(cmd, "remove") == 0 && n == 3) {
                        remover_vizinho(node, id_temp);
                    }
                   
                    else {
                        printf("[Erro] Uso correto: r <id> ou remove edge <id>\n");
                    }
                }

    /* --- COMANDO: SHOW NEIGHBORS ---*/
                    else if (((strcmp(cmd, "show") == 0) && (strcmp(net_temp, "neighbors") == 0)) || (strcmp(cmd, "sg") == 0)) {
                        
                        if (strcmp(cmd, "sg") == 0 && n == 1) {
                            show_neighbors(node);
                        }
                        else if (strcmp(cmd, "show") == 0 && n == 2) {
                            show_neighbors(node);
                        }
                        else {
                            printf("[Erro] Uso correto: sg ou show neighbors\n");
                        }
                    }
    /* --- COMANDO: HELP --- */
                else if((strcmp(cmd, "help") == 0 || strcmp(cmd, "h") == 0)) {
                    help(); // Exibe a mensagem de ajuda
                }
    /* --- COMANDO ANNOUCE --- */
                else if((strcmp(cmd, "announce") == 0 || strcmp(cmd, "a") == 0)) {
                    if( n == 1) {
                       
                        if(node->self.id[0] != '\0')/*Serve tanto para o join como para o direct join*/ {
                        announce(node);
                    } else {
                        printf("[Aviso] O nó precisa estar registado para anunciar os vizinhos.\n");
                    }
                    }
                    
                    else {
                        printf("[Erro] Uso correto: announce (a)\n");
                    }
                }
    /* --- COMANDO: SHOW ROUTING --- */
                else if (((strcmp(cmd, "show") == 0) && (strcmp(net_temp, "routing") == 0)) || (strcmp(cmd, "sr") == 0)) {
                    
                    if (strcmp(cmd, "sr") == 0 && n == 2) {
                        show_routing_table(node, net_temp);
                    }
                    else if (strcmp(cmd, "show") == 0 && n == 3) {
                        show_routing_table(node, id_temp);
                    }
                    else {
                        printf("Uso: show routing (sr) dest\n"); // Mensagem de erro atualizada para o formato exato
                    }
                }
    /* --- COMANDO: DIRECT JOIN --- */
                else if ((strcmp(cmd, "direct") == 0 && strcmp(net_temp, "join") == 0) || strcmp(cmd, "dj") == 0) {
                    char arg_net[16], arg_id[4];
                    int lido;
                    
                    // Extrai os argumentos consoante o formato usado (curto ou longo)
                    if (strcmp(cmd, "dj") == 0) {
                        lido = sscanf(buffer, "%*s %15s %3s", arg_net, arg_id);
                    } else {
                        lido = sscanf(buffer, "%*s %*s %15s %3s", arg_net, arg_id);
                    }
                    if (lido == 2) {
                        if(node->is_registered) {
                            printf("[Aviso] A nave já está ativa. Use 'leave' antes de mudar de identidade.\n");
                        } 
                        
                        else if (strlen(arg_net) != 3 || strlen(arg_id) != 2) {
                            printf("[Erro] Formato inválido. A rede deve ter 3 dígitos e o ID 2 dígitos.\n");
                        }
                        
                        else {
                            strcpy(node->self.id, arg_id);
                            strcpy(node->self.net, arg_net);
                            direct_join(node); 
                        }
                    } else {
                        printf("[Erro] Uso correto: dj <net> <id> ou direct join <net> <id>\n");
                    }
                }
    /* --- COMANDO: DIRECT ADD EDGE --- */
                else if ((strcmp(cmd, "direct") == 0 && strcmp(net_temp, "add") == 0) || strcmp(cmd, "dae") == 0) {
                    char arg_id[4], arg_ip[32], arg_port[8];
                    int lido;
                    
                    // Extrai os argumentos consoante o formato usado (curto ou longo)
                    if (strcmp(cmd, "dae") == 0) {
                        lido = sscanf(buffer, "%*s %3s %31s %7s", arg_id, arg_ip, arg_port);
                    } else {
                        lido = sscanf(buffer, "%*s %*s %*s %3s %31s %7s", arg_id, arg_ip, arg_port);
                    }
                    
                    if (lido == 3) {
                        direct_add_edge(node, arg_id, arg_ip, arg_port);
                    } else {
                        printf("[Erro] Uso correto: dae <id> <IP> <TCP> ou direct add edge <id> <IP> <TCP>\n");
                    }
                }
    /* --- COMANDO: MESSAGE --- */
                else if ((strcmp(cmd, "message") == 0 || strcmp(cmd, "m") == 0) && n >= 2) {
                    char dest_str[4];
                    char msg_texto[128] = "";
                    
                    if (sscanf(buffer, "%*s %s %[^\n]", dest_str, msg_texto) == 2) {
                        
                        int dest_int = atoi(dest_str);
                        int succ_id = node->routing_table.succ[dest_int];

                        if (succ_id != -1 && node->routing_table.state[dest_int] == 0) {
                            
                            
                            char pacote[256];
                            snprintf(pacote, sizeof(pacote), "CHAT %s %s %s\n", node->self.id, dest_str, msg_texto);

                            int enviou = 0;
                            for (int i = 0; i < node->num_neighbors; i++) {
                                if (atoi(node->neighbors[i].id) == succ_id) {
                                    enviar_mensagem_tcp(node->neighbors[i].fd, pacote);
                                    printf("-> Mensagem enviada para %s via vizinho %02d.\n", dest_str, succ_id);
                                    enviou = 1;
                                    break;
                                }
                            }
                            
                            if (!enviou) {
                                printf("[Erro Crítico] O vizinho de expedição %02d não está na tabela de ligações ativas.\n", succ_id);
                            }
                            
                        } else {
                            printf("[Erro] Não é possível enviar. Rota para %s é inexistente ou está em quarentena (COORD).\n", dest_str);
                        }
                    } else {
                        printf("[Erro] Uso correto: m <dest> <mensagem>\n");
                    }
                }
    /* --- COMANDO: START MONITOR --- */
                else if ((strcmp(cmd, "start") == 0 && strcmp(net_temp, "monitor") == 0) || strcmp(cmd, "sm") == 0) {
                    node->monitor = 1;
                    printf("[Sistema] Monitor de encaminhamento LIGADO.\n");
                }
    /* --- COMANDO: END MONITOR --- */
                else if ((strcmp(cmd, "end") == 0 && strcmp(net_temp, "monitor") == 0) || strcmp(cmd, "em") == 0) {
                    node->monitor = 0;
                    printf("[Sistema] Monitor de encaminhamento DESLIGADO.\n");
                }
    /* --- COMANDO NÃO RECONHECIDO ---*/
            else {
                printf("mensagem: %s\n", cmd);
                printf("[Aviso] Comando desconhecido. Digite 'help' (ou 'h') para ver os comandos disponíveis.\n");
            }
    

        
}return 0;}

/* -------------------------------------------------------------------------
 * FUNÇÃO: exibir_prompt
 * Descrição: Exibe o prompt da linha de comandos com o estado atual do nó,
 * de forma minimalista para não poluir o terminal.
 * Argumentos: 
 * - node: Estado global do nó.
 * Retorno: void
 * ------------------------------------------------------------------------- */
void exibir_prompt(NodeState *node) {
    printf("\n[Nó %s | %s | Rede %s] (Ajuda: 'help')\n-> ", 
           node->self.id[0] ? node->self.id : "?",
           node->is_registered ? "REGISTADO" : "LIVRE",
           node->self.net[0] ? node->self.net : "?");
    fflush(stdout); 
}

/* -------------------------------------------------------------------------
 * FUNÇÃO: ler_linha_tcp
 * Descrição: Lê dados do socket TCP byte a byte até encontrar uma quebra 
 * de linha ('\n') ou o limite do buffer, garantindo que não lê fragmentos 
 * da mensagem seguinte.
 * Argumentos: 
 * - fd: Descritor do socket.
 * - buffer: Array para armazenar a mensagem lida.
 * - max_len: Tamanho máximo do buffer.
 * Retorno: O número de bytes lidos ou -1 em caso de erro.
 * ------------------------------------------------------------------------- */
ssize_t ler_linha_tcp(int fd, char *buffer, size_t max_len) {
    ssize_t n, total = 0;
    char c;

    while (total < max_len - 1) {
        n = read(fd, &c, 1);
        if (n == 1) {
            buffer[total++] = c;
            if (c == '\n') break; // Fim da mensagem
        } else if (n == 0) {
            return 0; // Conexão fechada pelo vizinho
        } else {
            return -1; // Erro de leitura
        }
    }
    buffer[total] = '\0';
    return total;
}
/* -------------------------------------------------------------------------
 * FUNÇÃO: processar_dados_vizinho
 * Descrição: Lê uma mensagem recebida de um vizinho e invoca o parser 
 * correspondente ao comando (ROUTE, COORD, UNCOORD, CHAT, NEIGHBOR).
 * Trata também da deteção de desconexões abruptas do socket.
 * Argumentos: 
 * - node: Estado global do nó.
 * - index: Índice do vizinho na tabela de ligações ativas.
 * Retorno: void
 * ------------------------------------------------------------------------- */
void processar_dados_vizinho(NodeState *node, int index) {
    char buffer[256];
    int fd = node->neighbors[index].fd;
    ssize_t n = ler_linha_tcp(fd, buffer, sizeof(buffer));

    //  Detetar desconexão
    if (n <= 0) {
        printf("[Sistema] O vizinho %s desconectou-se.\n", node->neighbors[index].id);
        remover_vizinho(node, node->neighbors[index].id);
        
        return;
    }

    if (node->monitor == 1) {
        // Se a mensagem começar por ROUTE, COORD ou UNCOORD, mostramos no monitor (porque são mensagens de encaminhamento)
        if (strncmp(buffer, "ROUTE", 5) == 0 || strncmp(buffer, "COORD", 5) == 0 || strncmp(buffer, "UNCOORD", 7) == 0) {
            
            printf("[Monitor] Mensagem recebida de %s: %s", node->neighbors[index].id, buffer);
        }
    }
    char id_orig[4], id_dest[4], msg[128];
    
    // Identificar o comando recebido
    if (sscanf(buffer, "NEIGHBOR %s", id_orig) == 1 && strnlen(id_orig, sizeof(id_orig)) == 2 ){
        
        strncpy(node->neighbors[index].id, id_orig, sizeof(node->neighbors[index].id) - 1);
        node->neighbors[index].id[sizeof(node->neighbors[index].id) - 1] = '\0'; 
       
        node->last_udp_tid = gerar_tid(node, 0);
        char message[128];
        snprintf(message, sizeof(message), "CONTACT %03d 0 %s %s\n", 
         node->last_udp_tid, node->self.net, id_orig);
        enviar_mensagem_udp(node->net.udp_fd, node->net.udp_res, message);
        
        partilhar_tabela_com_vizinho(node, node->neighbors[index].fd);
    }
   else if (strncmp(buffer, "ROUTE", 5) == 0) {
        int dist_route;
        if (sscanf(buffer, "ROUTE %s %d", id_orig, &dist_route) == 2) {
           
            processar_Route(node, buffer, index); // Passa o index, não o fd!
        }
    }
    // ---> Intercetar COORD <---
    else if (strncmp(buffer, "COORD", 5) == 0) {
        process_Coord(node, buffer, index);
    }
    // ---> Intercetar UNCOORD <---
    else if (strncmp(buffer, "UNCOORD", 7) == 0) {
        process_Uncoord(node, buffer, index);
    }
    else if (sscanf(buffer, "CHAT %s %s %[^\n]", id_orig, id_dest, msg) == 3) {
        // Se a mensagem é para nós
        if (strcmp(id_dest, node->self.id) == 0) {
            printf("\n[CHAT] Mensagem de %s: %s\n", id_orig, msg);
        } else {
            // Guardamos logo os valores para não repetir atoi()
            int dest_int = atoi(id_dest);
            int succ_id = node->routing_table.succ[dest_int];

            // Verifica se a rota existe e não está congelada
            if (succ_id != -1 && node->routing_table.state[dest_int] == 0) {
                printf("[Encaminhamento] A retransmitir de %s para %s via vizinho %02d...\n", 
                       id_orig, id_dest, succ_id);
                
                for (int i = 0; i < node->num_neighbors; i++) {
                    if (atoi(node->neighbors[i].id) == succ_id) {
                        // Reenvia o pacote original diretamente
                        enviar_mensagem_tcp(node->neighbors[i].fd, buffer); 
                        break;
                    }
                }
            } else {
                printf("[Erro] Rota para %s inexistente/congelada. Mensagem descartada.\n", id_dest);
            }
        }
    }
    else {
        printf("[Aviso] Mensagem desconhecida do vizinho %s: %s\n", node->neighbors[index].id, buffer);
    }
}
/* -------------------------------------------------------------------------
 * FUNÇÃO: init_routing_table
 * Descrição: Inicializa a tabela de encaminhamento com valores por defeito
 * (distâncias no infinito e nenhum sucessor definido).
 * Argumentos: 
 * - node: Estado global do nó.
 * - num_nodes: Número máximo de nós na rede (MAX_NODES).
 * Retorno: void
 * ------------------------------------------------------------------------- */
void init_routing_table(NodeState *node, int num_nodes) {
    for(int i=0; i<num_nodes; i++){
        node->routing_table.dist[i] = INF; // Distância desconhecida
        node->routing_table.succ[i] = -1; // Sucessor desconhecido
        node->routing_table.state[i] = 0; // Estado inicial (0 = desconhecido)
        node->routing_table.succ_coord[i] = -1; // Coordenada do sucessor desconhecida
        for(int j=0; j<MAX_NEIGHBORS; j++){
            node->routing_table.coord[i][j] = 0; // Coordenadas dos vizinhos desconhecidas
        }
    }
}
/* -------------------------------------------------------------------------
 * FUNÇÃO: partilhar_tabela_com_vizinho
 * Descrição: Varre a tabela de encaminhamento e envia todas as rotas
 * válidas (distância != INF e estado normal) a um vizinho recém-ligado.
 * Argumentos: 
 * - node: Estado global do nó.
 * - fd_vizinho: Descritor do socket TCP do novo vizinho.
 * Retorno: void
 * ------------------------------------------------------------------------- */
void partilhar_tabela_com_vizinho(NodeState *node, int fd_vizinho) {
    char msg_route[64];
    if(fd_vizinho < 0) {
        printf("[Erro] Socket inválido para partilhar a tabela de encaminhamento.\n");
        return;
    }
    // Percorre todos os destinos possíveis
    for (int i = 0; i < MAX_NODES; i++) {
        // Se conhecemos um caminho para este destino e NÃO estamos bloqueados a coordenar
        if (node->routing_table.dist[i] != INF && node->routing_table.state[i] == 0) {
            
            // Envia a rota para o novo vizinho
            snprintf(msg_route, sizeof(msg_route), "ROUTE %02d %d\n", i, node->routing_table.dist[i]);
            enviar_mensagem_tcp(fd_vizinho, msg_route);
        }
    }
    if (node->monitor) {
        printf("[Monitor] Tabela de encaminhamento partilhada com o novo vizinho.\n");
    }
}
/* -------------------------------------------------------------------------
 * FUNÇÃO: gerar_tid
 * Descrição: Gera um Identificador de Transação (TID) aleatório entre 
 * 000 e 999 para identificar as mensagens UDP enviadas ao Servidor de Nós.
 * Argumentos: 
 * - node: Estado global do nó.
 * - N: Parâmetro auxiliar.
 * Retorno: O valor inteiro do TID gerado.
 * ------------------------------------------------------------------------- */
int gerar_tid(NodeState *node, int N) {
    char tid_str[8];
    snprintf(tid_str, sizeof(tid_str), "%03d", rand() % 1000);
    return atoi(tid_str); 
  
}



