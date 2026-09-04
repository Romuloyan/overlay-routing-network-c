#ifndef STRUCTS_H
#define STRUCTS_H


#include "lib.h"
#define MAX_NODES 100
#define MAX_NEIGHBORS 99
#define INF 101

typedef struct {
    int dist[MAX_NODES];
    int succ[MAX_NODES];
    int state[MAX_NODES];
    int succ_coord[MAX_NODES];
    int coord[MAX_NODES][MAX_NODES];
   
} RoutingTable;

typedef struct {
    char id[4];      // ID do nó (00-99 + '\0')
    char ip[32];     // O nosso IP
    char port[8];    // O nosso porto TCP
    char net[4];
} NodeIdentity;

typedef struct {
    int udp_fd;
    struct addrinfo *udp_res;
    int tcp_server_fd; // Para aceitar ligações de vizinhos
      
} NetResources;
typedef struct{
    char id[4];      // ID do nó vizinho (00-99 + '\0')
    int fd;           // Descritor do socket do vizinho
    char ip[32];     // IP do vizinho
    char port[8];    // Porto TCP do vizinho
} Neighbor;

typedef struct {
    int dist;              // dist[t]: distância ao destino t
    int succ;              // succ[t]: ID do vizinho de expedição (INT em vez de char)
    int state;             // state[t]: 0 (expedição) ou 1 (coordenação)
    int succ_coord;        // succ_coord[t]: quem iniciou a coordenação (-1 se fomos nós)
    int coord[MAX_NODES];  // coord[t,j]: 1 se à espera, 0 se terminada
    
} RoutingEntry;

typedef struct {
    NodeIdentity self;
    NetResources net;
    int is_registered; // Flag: 0 ou 1
    Neighbor neighbors[MAX_NEIGHBORS];
    int num_neighbors; // Número de vizinhos atualmente na tabela
    int last_udp_tid; // Para armazenar o último TID gerado, se necessário
    RoutingTable routing_table; // Tabela de encaminhamento (a implementar no futuro)
    int monitor; // 0 = silencioso, 1 = ativo para debug
} NodeState;

typedef struct{
    char *regIP;
    char *regPort;
} RegServerInfo;

#endif 