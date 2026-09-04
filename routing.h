#ifndef ROUTING_H
#define ROUTING_H

#include "lib.h"

void processar_Route(NodeState *node, char *buffer, int vizinho_index);
void coordenation_sender(NodeState *node, int vizinho_index);
void process_Coord(NodeState *node, char *buffer, int vizinho_index);
void process_Uncoord(NodeState *node, char *buffer, int vizinho_index);
void verificar_fim_coordenacao(NodeState *node, int dest);

#endif
