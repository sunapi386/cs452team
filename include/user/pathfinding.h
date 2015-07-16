#ifndef __PATHFINDING_H
#define __PATHFINDING_H

struct track_node;
struct track_edge;

struct track_edge *getNextEdge(struct track_node *node);

struct track_node *getNextNode(struct track_node *currentNode);

struct track_node *getNextSensor(struct track_node *node);

int distanceBetween(struct track_node *from, struct track_node *to);

#endif
