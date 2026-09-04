#include "lib.h"

/* -------------------------------------------------------------------------
 * FUNÇÃO: enviar_mensagem_udp
 * Descrição: Envia um datagrama UDP para o destino especificado.
 * Argumentos: 
 * - fd: Descritor do socket UDP.
 * - res: Estrutura com o endereço de destino resolvido.
 * - message: Conteúdo da mensagem a enviar.
 * Retorno: void
 * ------------------------------------------------------------------------- */
void enviar_mensagem_udp(int fd, struct addrinfo *res, const char *message) {
    ssize_t n;
    
    n = sendto(fd, message, strlen(message), 0, res->ai_addr, res->ai_addrlen);
    if(n == -1) /*error*/ exit(1);
}

/* -------------------------------------------------------------------------
 * FUNÇÃO: receber_mensagem_udp
 * Descrição: Aguarda e lê uma mensagem do socket UDP, encaminhando-a para 
 * o parser de protocolo.
 * Argumentos: 
 * - fd: Descritor do socket UDP.
 * - node: Estado global para processamento da resposta.
 * Retorno: void
 * ------------------------------------------------------------------------- */
void receber_mensagem_udp(int fd, NodeState *node) {
    struct sockaddr addr;
    socklen_t addrlen;
    ssize_t n;
    char buffer[128+1];
    addrlen = sizeof(addr);

    n = recvfrom(fd, buffer, 128, 0, &addr, &addrlen);
    if(n < 0) /*error*/ exit(1);
    buffer[n] = '\0';
    
    tratar_mensagem_udp(buffer, node);
}

/* -------------------------------------------------------------------------
 * FUNÇÃO: configurar_conexao_udp
 * Descrição: Cria um socket UDP e resolve o endereço do servidor de nós.
 * Argumentos: 
 * - node: IP/Hostname do servidor.
 * - service: Porto/Serviço do servidor.
 * - fd: Ponteiro para armazenar o descritor criado.
 * - res: Ponteiro para armazenar a estrutura de endereço resolvida.
 * Retorno: void
 * ------------------------------------------------------------------------- */
void configurar_conexao_udp(const char *node, const char *service, int *fd, struct addrinfo **res) {
    struct addrinfo hints;
    int errcode;

    *fd = socket(AF_INET, SOCK_DGRAM, 0); // UDP socket
    if(*fd == -1) /*error*/ exit(1);

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP socket

    errcode = getaddrinfo(node, service, &hints, res);
    if(errcode != 0) /*error*/ exit(1);
}
/* -------------------------------------------------------------------------
 * FUNÇÃO: limpar_conexao
 * Descrição: Executa o fecho ordenado de todos os recursos de rede (UDP e TCP).
 * Argumentos: 
 * - node: Estado global do nó.
 * Retorno: void
 * Notas: Envia mensagem de saída ao servidor e encerra sockets de vizinhos.
 * ------------------------------------------------------------------------- */
void limpar_conexao(NodeState *node) {
    if(node->is_registered == 1) {
        leave(node); // Deixa a rede, se estiver registado
    }
    while (node->num_neighbors > 0) {
        remover_vizinho(node, node->neighbors[0].id); // Limpa os vizinhos, se houver algum
    }
    if (node->net.tcp_server_fd > 0) {
        close(node->net.tcp_server_fd);}

    // 4. Desmonta a antena UDP
    if (node->net.udp_res != NULL) {
        freeaddrinfo(node->net.udp_res);
    }
    if (node->net.udp_fd > 0) {
        close(node->net.udp_fd);
    }
}
/* -------------------------------------------------------------------------
 * FUNÇÃO: configurar_conexao_tcp_cliente
 * Descrição: Cria um socket TCP e resolve o endereço para uma ligação de saída.
 * Argumentos: 
 * - node: IP de destino.
 * - service: Porto de destino.
 * - fd: Ponteiro para o descritor.
 * - res: Ponteiro para a estrutura de endereço.
 * Retorno: 0 em caso de sucesso, -1 em caso de erro.
 * ------------------------------------------------------------------------- */
int configurar_conexao_tcp_cliente(const char *node, const char *service, int *fd, struct addrinfo **res) {
    struct addrinfo hints;
    int n;

    *res = NULL;
    *fd = socket(AF_INET, SOCK_STREAM, 0); // TCP socket
    if(*fd == -1) {
        perror("[Erro] Não foi possível criar socket TCP");
        return -1;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;       // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP socket

    n = getaddrinfo(node, service, &hints, res);
    if(n != 0) {
        fprintf(stderr, "[Erro] Endereço TCP inválido: %s\n", gai_strerror(n));
        close(*fd);
        *fd = -1;
        return -1;
    }
    return 0;
}
/* -------------------------------------------------------------------------
 * FUNÇÃO: conectar_tcp
 * Descrição: Estabelece uma ligação TCP a um destino previamente resolvido.
 * Argumentos: 
 * - fd: Descritor do socket TCP.
 * - res: Estrutura de endereço do destino.
 * Retorno: void
 * ------------------------------------------------------------------------- */
void conectar_tcp(int fd, struct addrinfo *res) {
    int n;
    n = connect(fd, res->ai_addr, res->ai_addrlen);
    if(n == -1) /*error*/ exit(1);
}
/* -------------------------------------------------------------------------
 * FUNÇÃO: configurar_conexao_tcp_server
 * Descrição: Configura o socket TCP para um servidor.
 * Argumentos: 
 * - node: Estado global do nó.
 * Retorno: void
 * ------------------------------------------------------------------------- */
void configurar_conexao_tcp_server(NodeState *node)  {
    struct addrinfo hints,*res; // Estrutura para resolver o endereço do servidor
    int reuse = 1;// Variável para a opção de reutilização de endereço

    memset(&hints,0,sizeof hints);
    hints.ai_family=AF_INET;//IPv4
    hints.ai_socktype=SOCK_STREAM;//TCP socket
    hints.ai_flags=AI_PASSIVE;

    // Usamos o porto que já está guardado na struct 
    if (getaddrinfo(NULL, node->self.port, &hints, &res) != 0) exit(1);

    if( (node->net.tcp_server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)/*error*/exit(1); // Criar o socket TCP para o servidor


    setsockopt(node->net.tcp_server_fd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(int)); // Permite reutilizar o porto imediatamente após o programa terminar

    if(bind(node->net.tcp_server_fd,res->ai_addr,res->ai_addrlen)==-1)/*error*/exit(1);
    if(listen(node->net.tcp_server_fd,SOMAXCONN/*O numero de conexões em espera*/)==-1)/*error*/exit(1);
    freeaddrinfo(res); // Libera a memória alocada por getaddrinfo

}
/* ------------------------------------------------------------------------
 * FUNÇÃO: aceitar_conexoes_tcp
 * Descrição: Aceita novas conexões TCP de vizinhos.
 * Argumentos: 
 * - node: Estado global do nó.
 * Retorno: void
 * ------------------------------------------------------------------------- */
void aceitar_conexoes_tcp(NodeState *node) {
    /*Declaração de variáveis*/
    int newfd;
    struct sockaddr addr; socklen_t addrlen = sizeof(addr);
    
    if((newfd=accept(node->net.tcp_server_fd,&addr,&addrlen))==-1)  /*error*/exit(1);

    if(node->num_neighbors < MAX_NEIGHBORS) {
        node->neighbors[node->num_neighbors].fd = newfd;
        node->neighbors[node->num_neighbors].id[0] = '\0'; // ID ainda desconhecido, será atualizado quando recebermos a mensagem NEIGHBOR do vizinho

        node->num_neighbors++; 
    } else {
        printf("[Aviso] Tabela de vizinhos cheia. Não é possível aceitar mais conexões.\n");
        close(newfd); // Fecha a conexão se a tabela de vizinhos estiver cheia
    }
    }


/* -------------------------------------------------------------------------
 * FUNÇÃO: enviar_mensagem_tcp
 * Descrição: Envia uma mensagem através de um socket TCP.
 * Argumentos: 
 * - fd: Descritor do socket TCP.
 * - mensagem: Ponteiro para a mensagem a enviar.
 * Retorno: void
 * ------------------------------------------------------------------------- */
void enviar_mensagem_tcp(int fd, const char *mensagem) {
    
    ssize_t nbytes, nleft, nwritten;
    const char *ptr;

    nbytes = strlen(mensagem);
    ptr = mensagem;
    nleft = nbytes;

    while(nleft > 0) {
        nwritten = write(fd, ptr, nleft);
        if(nwritten <= 0) /*error*/ exit(1);
        
        nleft -= nwritten;
        ptr += nwritten; // Avança o ponteiro para o resto da mensagem
    }
}

/* -------------------------------------------------------------------------
 * FUNÇÃO: neighbor_edge_create
 * Descrição: Cria uma aresta de vizinho, estabelecendo uma ligação TCP.
 * Argumentos: 
 * - node: Estado global do nó.
 * Retorno: int
 * ------------------------------------------------------------------------- */
int neighbor_edge_create(NodeState *node) {

    int sock_fd;
    struct addrinfo *res;
    
    //  Configura e liga (usa as tuas funções de cliente TCP)
    if (configurar_conexao_tcp_cliente(node->neighbors[node->num_neighbors].ip,
                                       node->neighbors[node->num_neighbors].port,
                                       &sock_fd, &res) != 0) {
        return -1;
    }
    
    
    struct timeval timeout;
    timeout.tv_sec = 3;  // 3 segundos de limite de paciência
    timeout.tv_usec = 0; // 0 microsegundos
    // Aplica o timeout para operações de envio (afeta o connect em Linux/Unix)
    setsockopt(sock_fd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
    // Aplica o timeout para operações de leitura (afeta o read/recv)
    setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    
    if (connect(sock_fd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("[Erro] Falha ao ligar ao vizinho");
        close(sock_fd);
        freeaddrinfo(res);
        return -1;
    }
   node->neighbors[node->num_neighbors].fd = sock_fd; // Armazena o fd do vizinho na tabela de vizinhos
   printf("[Sistema] Conexão TCP estabelecida com o vizinho %s (fd: %d).\n", node->neighbors[node->num_neighbors].id, sock_fd); 
   // Envia a identificação: NEIGHBOR id<LF> 
    char greeting[32];
    snprintf(greeting, sizeof(greeting), "NEIGHBOR %s\n", node->self.id);
    enviar_mensagem_tcp(sock_fd, greeting);
    if(node->monitor == 1) {
        printf("[Monitor] Mensagem TCP enviada para %s: %s", node->neighbors[node->num_neighbors].id, greeting);
    }  
    
    partilhar_tabela_com_vizinho(node, sock_fd); // Compartilha a tabela de encaminhamento com o novo vizinho
    freeaddrinfo(res); // Libera a memória alocada por getaddrinfo
    return 1; // Sucesso
}

