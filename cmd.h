#ifndef CMD_H
#define CMD_H

#include "lib.h"
void leave(NodeState *node);
void help();
void join(NodeState *node);
void nodes_querry(NodeState *node, char *net_to_check);
void show_neighbors(NodeState *node);
void add_edge(NodeState *node, const char *id);
void show_routing_table(NodeState *node, char *dest_id);
void announce(NodeState *node);
int tratar_mensagem_udp(char *buffer, NodeState *node);
void direct_join(NodeState *node);
void direct_add_edge(NodeState *node, char *id_vizinho, char *ip_vizinho, char *port_vizinho);
void enviar_chat(NodeState *node, char *dest_id, char *msg_texto);
void remover_vizinho(NodeState *node, char *id_alvo);


#endif 