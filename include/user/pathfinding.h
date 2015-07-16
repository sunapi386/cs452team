#ifndef __PATHFINDING_H
#define __PATHFINDING_H

struct track_node;

struct Position {
    struct track_node *node;
    int offset;
};

void plan_route(struct track_node *start, struct track_node *end);

int distanceBetween(struct track_node *from, struct track_node *to);
struct track_edge *getNextEdge(struct track_node *node);
struct track_node *getNextNode(struct track_node *currentNode);
struct track_node *getNextSensor(struct track_node *node);

#endif
