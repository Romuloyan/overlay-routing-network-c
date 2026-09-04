#ifndef CONNECT_H
#define CONNECT_H

#include "lib.h"
void enviar_mensagem_udp(int fd, struct addrinfo *res, const char *message);
void receber_mensagem_udp(int fd, NodeState *node);
void configurar_conexao_udp(const char *node, const char *service, int *fd, struct addrinfo **res);
void limpar_conexao(NodeState *node);

void configurar_conexao_tcp_cliente(const char *node, const char *service, int *fd, struct addrinfo **res);
void aceitar_conexoes_tcp(NodeState *node);
void configurar_conexao_tcp_server(NodeState *nodes);
void conectar_tcp(int fd, struct addrinfo *res);
void enviar_mensagem_tcp(int fd, const char *mensagem);
void processar_mensagem_tcp(NodeState *node, char *buffer, int fd);
int neighbor_edge_create(NodeState *node);


#endif // CONNECT_H