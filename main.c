/* -------------------------------------------------------------------------
 * PROJETO: OWR - Overlay With Routing
 * FICHEIRO: main.c
 * DESCRIÇÃO: Ponto de entrada do sistema. Gere a multiplexagem de entrada/saída
 * (teclado, UDP e múltiplas ligações TCP) através da função select().
 * ------------------------------------------------------------------------- */
#include "lib.h"
#include "structs.h"




int main(int argc, char *argv[] ) {
    /* Inicialização do gerador de números aleatórios */
    srand(time(NULL));
    /*Declaração de estruturas*/
    NodeState node; // Estrutura para manter o estado do nó
    RegServerInfo regInfo; // Estrutura para armazenar as informações do servidor de registo
    memset(&node, 0, sizeof(NodeState)); 
    node.monitor = 0;
    init_routing_table(&node, MAX_NODES); // Inicializa a tabela de encaminhamento
    
    /*Setup inicial*/
    processar_argumentos(argc, argv, &node, &regInfo); 
    configurar_conexao_udp(regInfo.regIP, regInfo.regPort, &node.net.udp_fd, &node.net.udp_res); // Configura a conexão UDP para o servidor de registo
    configurar_conexao_tcp_server(&node); // Configura o socket TCP para aceitar conexões de vizinhos

    printf("[Sistema] Nó pronto. IP: %s | Porto TCP: %s\n", node.self.ip, node.self.port);
    
    node.self.id[0] = '\0'; // ID começa vazio, será atribuído pelo servidor de registo

    while(1){
        // Exibe o prompt a cada iteração do loop, para que o utilizador saiba que pode digitar um comando
        exibir_prompt(&node);
    
        fd_set read_fds; // Conjunto de descritores para a função select
        FD_ZERO(&read_fds); // Inicializa o conjunto de descritores
        
        /*Teclado*/ 
        FD_SET(STDIN_FILENO, &read_fds); // Adiciona o descritor do teclado ao conjunto de leitura
        int max_fd = STDIN_FILENO;
       
        /*UDP*/
        FD_SET(node.net.udp_fd, &read_fds); // Adiciona o socket UDP ao conjunto de leitura)
        if(node.net.udp_fd > max_fd) max_fd = node.net.udp_fd;

        /*TCP*/
        FD_SET(node.net.tcp_server_fd, &read_fds); // Adiciona o socket TCP ao conjunto de leitura (para aceitar conexões de vizinhos)
       if(node.net.tcp_server_fd > max_fd) max_fd = node.net.tcp_server_fd;

        /*vizinhos*/
        for (int i = 0; i < node.num_neighbors; i++) {
            FD_SET(node.neighbors[i].fd, &read_fds);
            if (node.neighbors[i].fd > max_fd) max_fd = node.neighbors[i].fd;
        }

        /*Uso do select */
        int activity = select(max_fd +1, &read_fds, NULL, NULL, NULL); // Espera por atividade em qualquer descritor
        if(activity <=0){perror("select error"); break;}

        /*Verificação de atividade*/
        /*TCP*/
        if(FD_ISSET(node.net.tcp_server_fd, &read_fds)) {
            //printf("[Sistema] Nova conexão TCP detectada. Aceitando conexões de vizinhos...\n");
            //receber_mensagem_tcp(&node, node.net.tcp_server_fd, 0); // Aceita a nova conexão TCP (o número de bytes esperados é irrelevante para o accept)
            aceitar_conexoes_tcp(&node);
            }

        /*Teclado*/
        if(FD_ISSET(STDIN_FILENO, &read_fds)){
            if(teclado(&node, regInfo) ==1) {
                break; // Se a função teclado retornar 1, significa que o comando "exit" foi executado e devemos sair do loop
            }
        }
        /*UDP*/
        if (FD_ISSET(node.net.udp_fd, &read_fds)) {
            // Chamamos a tua função de receção para ler a resposta
            receber_mensagem_udp(node.net.udp_fd, &node);
            //node.is_registered = 1; 
        }
        for (int i = 0; i < node.num_neighbors; i++) {
            if (FD_ISSET(node.neighbors[i].fd, &read_fds)) {
                
                processar_dados_vizinho(&node, i); 
            }
        }
    

    }
    return 0;
}