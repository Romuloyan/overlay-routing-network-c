#include "lib.h"
/* -------------------------------------------------------------------------
 * FUNÇÃO: processar_Route
 * Descrição: Processa mensagens ROUTE recebidas. Se detetar uma rota mais 
 * curta para o destino, atualiza a tabela de encaminhamento e propaga a 
 * nova distância aos restantes vizinhos (regra de Split Horizon).
 * Argumentos: 
 * - node: Estado global do nó.
 * - buffer: Conteúdo da mensagem.
 * - vizinho_index: Índice do vizinho remetente na tabela de ligações.
 * Retorno: void
 * ------------------------------------------------------------------------- */
void processar_Route(NodeState *node, char *buffer, int vizinho_index) {
    char dest_str[4];
    int n;

    if (vizinho_index < 0 || vizinho_index >= node->num_neighbors) {
        return;
    }

    // 1. Parse the incoming ROUTE message: "ROUTE dest n"
    int t;
    if (sscanf(buffer, "ROUTE %3s %d", dest_str, &n) == 2 &&
        parse_node_id(dest_str, &t) && n >= 0 && n <= INF) {
        int j;
        if (!parse_node_id(node->neighbors[vizinho_index].id, &j)) {
            return;
        }
        printf("[Encaminhamento] Recebido ROUTE para %d com distância %d do vizinho %s\n", 
               t, n, node->neighbors[vizinho_index].id);

     
        if (n + 1 < node->routing_table.dist[t]) {
            
            // 3. Atualiza a estimativa de distância e define j como vizinho de expedição [cite: 81]
            node->routing_table.dist[t] = n + 1;
            node->routing_table.succ[t] = j; 

            printf("[Protocolo] Rota otimizada para %02d! Nova distância: %d via vizinho %02d\n", 
                   t, node->routing_table.dist[t], j);
                   // 4. Se o nó estiver no estado normal de expedição, propaga a melhoria 
            if (node->routing_table.state[t] == 0)// Se o estado é normal (0), propaga a melhoria
            {
                char msg_out[64];
                snprintf(msg_out, sizeof(msg_out), "ROUTE %02d %d\n", t, node->routing_table.dist[t]);
                for (int i = 0; i < node->num_neighbors; i++) {
                    if (i != vizinho_index) { // Não envia de volta para o vizinho que enviou a mensagem
                        
                        enviar_mensagem_tcp(node->neighbors[i].fd, msg_out);
                        printf("[Protocolo] Propagada melhoria de rota para %02d via vizinho %s\n", 
                               t, node->neighbors[i].id);
                    }
                }
            }
        }
    }
        else {
            printf("[Erro] Mensagem ROUTE mal formatada: %s\n", buffer);
        }
}
/* -------------------------------------------------------------------------
 * FUNÇÃO: coordenation_sender
 * Descrição: Acionada quando uma ligação a um vizinho cai ou é encerrada.
 * Varre a tabela de encaminhamento e, para cada destino cujo sucessor era o 
 * nó removido, coloca a rota em estado de coordenação (quarentena) e 
 * difunde o alarme (COORD) aos restantes vizinhos ativos.
 * Argumentos: 
 * - node: Estado global do nó.
 * - vizinho_index: Índice do vizinho que foi desconectado na tabela local.
 * Retorno: void
 * ------------------------------------------------------------------------- */
void coordenation_sender(NodeState *node, int vizinho_index) {
    // 1. Extrair o ID real do vizinho para podermos comparar com a tabela
    int id_removido;
    if (vizinho_index < 0 || vizinho_index >= node->num_neighbors ||
        !parse_node_id(node->neighbors[vizinho_index].id, &id_removido)) {
        return;
    }
    
    for( int i = 0; i < MAX_NODES; i++){
        if(node->routing_table.succ[i] == id_removido) {
            node->routing_table.state[i] = 1;       // Tranca o destino (Estado 1)
            node->routing_table.dist[i] = INF;      // Rota destruída
            node->routing_table.succ[i] = -1;       // Fica sem sucessor
            node->routing_table.succ_coord[i] = -1; // Fomos nós que detetámos a quebra

            
            char msg_coord[64];
            snprintf(msg_coord, sizeof(msg_coord), "COORD %02d\n", i);
        
            for (int k = 0; k < node->num_neighbors; k++) {
                if (k != vizinho_index) { // Não envia de volta para o vizinho que detectou a falha
                    int viz_restante;
                    if (!parse_node_id(node->neighbors[k].id, &viz_restante)) {
                        continue;
                    }
                    enviar_mensagem_tcp(node->neighbors[k].fd, msg_coord);
                    if(node->monitor == 1) {
                        printf("[Monitor] Enviado para o vizinho %s para o destino %02d a mensagem: %s\n", node->neighbors[k].id, i, msg_coord);
                    }
                    // Assinala que ficamos à espera da resposta (UNCOORD) deste vizinho
                    node->routing_table.coord[i][viz_restante] = 1; 
                }
            } verificar_fim_coordenacao(node, i);
        }   
        else{
            node->routing_table.coord[i][id_removido] = 0; // Marca que a coordenação para este destino está terminada
        }
    
}}

/* -------------------------------------------------------------------------
 * FUNÇÃO: process_Coord
 * Descrição: Processa mensagens de alarme COORD recebidas de um vizinho.
 * Avalia o impacto da falha de acordo com o estado atual da rota para o 
 * destino e reage em conformidade (ignora, ajuda com rota alternativa ou 
 * entra também em coordenação).
 * Argumentos: 
 * - node: Estado global do nó.
 * - buffer: Conteúdo da mensagem recebida.
 * - vizinho_index: Índice do vizinho remetente na tabela local.
 * Retorno: void
 * ------------------------------------------------------------------------- */
void process_Coord(NodeState *node, char *buffer, int vizinho_index) {
    char dest_str[4];
    if (vizinho_index < 0 || vizinho_index >= node->num_neighbors) {
        return;
    }
    
    int dest;
    if (sscanf(buffer, "COORD %3s", dest_str) == 1 &&
        parse_node_id(dest_str, &dest)) {
        
        int j;
        if (!parse_node_id(node->neighbors[vizinho_index].id, &j)) {
            return;
        }
        if(node->monitor == 1) {
            
            printf("Mensagem TCP recebida de %s: %s\n", node->neighbors[vizinho_index].id, buffer);
        }
        //Se Nos estamos em coordenação para este destino, registamos que este vizinho já nos deu luz verde
        if (node->routing_table.state[dest] == 1) { 
            
            if (node->routing_table.succ[dest] == j) {
                node->routing_table.dist[dest] = INF;
                node->routing_table.succ[dest] = -1;
                
            }
                char msg_exped[64];
            snprintf(msg_exped, sizeof(msg_exped), "UNCOORD %02d\n", dest);
            enviar_mensagem_tcp(node->neighbors[vizinho_index].fd, msg_exped);
            if(node->monitor == 1) {
                printf("[Mensagem] Enviado para o vizinho %s para o destino %02d a mensagem: %s\n", node->neighbors[vizinho_index].id, dest, msg_exped);
            }
            verificar_fim_coordenacao(node, dest);
        }// Se estamos no estado normal (0) e o vizinho que enviou a mensagem NÃO é o nosso sucessor para este destino, podemos enviar a nossa rota válida para ajudar na estabilização da rede
        else if (node->routing_table.state[dest] == 0 && node->routing_table.succ[dest] != j) { 
            char msg_route[64], msg_exped[64];
            
          
            // Envia a rota atual para o vizinho que está em coordenação, para ajudar na estabilização da rede
            snprintf(msg_route, sizeof(msg_route), "ROUTE %02d %d\n", dest, node->routing_table.dist[dest]);
            enviar_mensagem_tcp(node->neighbors[vizinho_index].fd, msg_route);
            if(node->monitor == 1) {
                printf("[Mensagem] Enviado para o vizinho %s para o destino %02d a mensagem: %s\n", node->neighbors[vizinho_index].id, dest, msg_route);
            }
            // Envia o UNCOORD para o vizinho que está em coordenação, para ajudar na estabilização da rede
            snprintf(msg_exped, sizeof(msg_exped), "UNCOORD %02d\n", dest);
            enviar_mensagem_tcp(node->neighbors[vizinho_index].fd, msg_exped);
            if(node->monitor == 1) {
                printf("[Monitor] Mensagem enviada para o vizinho %s para o destino %02d a mensagem: %s\n", node->neighbors[vizinho_index].id, dest, msg_exped);
            }
        }
        
        // Se estamos no estado normal (0) e o vizinho que enviou a mensagem é o nosso sucessor para este destino, entramos em coordenação imediatamente, pois é um sinal de que a rota que tínhamos para este destino foi comprometida
        else if (node->routing_table.state[dest] == 0 && node->routing_table.succ[dest] == j) { 
            

            node->routing_table.state[dest] = 1;                              
            node->routing_table.succ_coord[dest] = j; // Guardamos quem iniciou a coordenação para depois enviar o UNCOORD final
            node->routing_table.dist[dest] = INF;                             
            node->routing_table.succ[dest] = -1;                              

            
            char msg_coord[64];
            snprintf(msg_coord, sizeof(msg_coord), "COORD %02d\n", dest);
            
            for (int k = 0; k < node->num_neighbors; k++) {
                int viz_restante;
                if (!parse_node_id(node->neighbors[k].id, &viz_restante)) {
                    continue;
                }
                enviar_mensagem_tcp(node->neighbors[k].fd, msg_coord);
                
                // Assinala que ficamos à espera da resposta (UNCOORD) deste vizinho
                node->routing_table.coord[dest][viz_restante] = 1; 
            }
            
           // Verifica se por acaso já não tínhamos recebido um COORD deste vizinho para este destino, o que significaria que a rede já está estabilizada e podemos voltar à expedição imediatamente
            verificar_fim_coordenacao(node, dest);
        }
    }
}
/* -------------------------------------------------------------------------
 * FUNÇÃO: process_Uncoord
 * Descrição: Processa a receção de uma mensagem UNCOORD, que atua como 
 * luz verde de um vizinho indicando que o seu lado da rede está estabilizado 
 * para um dado destino. Regista a resposta e verifica se a coordenação 
 * local já pode ser dada como concluída.
 * Argumentos: 
 * - node: Estado global do nó.
 * - buffer: Conteúdo da mensagem.
 * - vizinho_index: Índice do vizinho remetente na tabela local.
 * Retorno: void
 * ------------------------------------------------------------------------- */
void process_Uncoord(NodeState *node, char *buffer, int vizinho_index) {
    char dest_str[4];
    if (vizinho_index < 0 || vizinho_index >= node->num_neighbors) {
        return;
    }
    
    int dest;
    if (sscanf(buffer, "UNCOORD %3s", dest_str) == 1 &&
        parse_node_id(dest_str, &dest)) {
        int j;
        if (!parse_node_id(node->neighbors[vizinho_index].id, &j)) {
            return;
        }

        
        // Se estamos congelados (Estado 1), registamos que este vizinho já nos deu luz verde
        if (node->routing_table.state[dest] == 1) { 
            node->routing_table.coord[dest][j] = 0; 
            verificar_fim_coordenacao(node, dest); 
        }
        if((node->routing_table.state[dest] == 1) && (node->routing_table.succ[dest] == j)){
            
            node->routing_table.state[dest] = 1;                              
            node->routing_table.succ_coord[dest] = j; 
            node->routing_table.dist[dest] = INF;                             
            node->routing_table.succ[dest] = -1;    

            verificar_fim_coordenacao(node, dest);     
                                
        }
        
    }
}
/* -------------------------------------------------------------------------
 * FUNÇÃO: verificar_fim_coordenacao
 * Descrição: Valida se o nó local já recebeu mensagens UNCOORD de todos os 
 * vizinhos dos quais estava à espera. Se sim, termina o estado de 
 * coordenação (quarentena), anuncia a sua melhor rota atualizada aos 
 * vizinhos e responde ao nó que inicialmente disparou o alarme.
 * Argumentos: 
 * - node: Estado global do nó.
 * - dest: ID do destino em avaliação.
 * Retorno: void
 * ------------------------------------------------------------------------- */
void verificar_fim_coordenacao(NodeState *node, int dest) {
    if (dest < 0 || dest >= MAX_NODES) {
        return;
    }
    // Se não estamos em coordenação para este destino, não fazemos nada
    if (node->routing_table.state[dest] != 1) return;

    // Verifica se algum vizinho ainda conectado NÃO respondeu (coord == 1)
    for (int k = 0; k < node->num_neighbors; k++) {
        int viz_id;
        if (!parse_node_id(node->neighbors[k].id, &viz_id)) {
            continue;
        }
        if (node->routing_table.coord[dest][viz_id] == 1) {
            return; // Ainda estamos à espera. A nave continua congelada.
        }
    }

    // Se chegámos aqui, todos responderam, Voltamos à Expedição.
    node->routing_table.state[dest] = 0;
    if (node->monitor) {
        printf("[Protocolo] Coordenação para o destino %02d terminada. Rede estabilizada.\n", dest);
    }
    
    // Se durante a espera aprendemos um caminho válido (!= INF), avisamos todos
    if (node->routing_table.dist[dest] != INF) {
        char msg_route[64];
        snprintf(msg_route, sizeof(msg_route), "ROUTE %02d %d\n", dest, node->routing_table.dist[dest]);
        for (int i = 0; i < node->num_neighbors; i++) {
            int viz_id;
            if (!parse_node_id(node->neighbors[i].id, &viz_id)) {
                continue;
            }
            if (viz_id != node->routing_table.succ[dest]) {
                enviar_mensagem_tcp(node->neighbors[i].fd, msg_route);
            
                if (node->monitor) {
                    printf("[Monitor] Mensagem enviada para %s: %s\n", node->neighbors[i].id, msg_route);
                }
            }   
        }
    }

    //Se foi iniciado por um vizinho específico, enviamos-lhe o UNCOORD final
    if (node->routing_table.succ_coord[dest] != -1) {
        char msg_exped[64];
        snprintf(msg_exped, sizeof(msg_exped), "UNCOORD %02d\n", dest);
        
        for (int i = 0; i < node->num_neighbors; i++) {
            int neighbor_id;
            if (parse_node_id(node->neighbors[i].id, &neighbor_id) &&
                neighbor_id == node->routing_table.succ_coord[dest]) {
                enviar_mensagem_tcp(node->neighbors[i].fd, msg_exped);
                if (node->monitor == 1) {
                    printf("[Monitor] Mensagem enviada para %s: %s", node->neighbors[i].id, msg_exped); 
                }
                break;
            }
        }
        
        // Limpa a memória de quem iniciou, pois já respondemos
        node->routing_table.succ_coord[dest] = -1;
    }
}
