#ifndef UTILS_H
#define UTILS_H

#include "lib.h"
void processar_argumentos(int argc, char *argv[], NodeState *node, RegServerInfo *regInfo);
int teclado( NodeState *node, RegServerInfo regInfo);
void exibir_prompt(NodeState *node);
void processar_dados_vizinho(NodeState *node, int index);
ssize_t ler_linha_tcp(int fd, char *buffer, size_t max_len);
void init_routing_table(NodeState *node, int num_nodes);
void partilhar_tabela_com_vizinho(NodeState *node, int vizinho_fd);
int gerar_tid(NodeState *node, int N);
#endif