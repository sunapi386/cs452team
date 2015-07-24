#ifndef __PATHFINDING_H
#define __PATHFINDING_H

struct track_node;
struct track_edge;
struct SensorClaim;

struct track_edge *getNextEdge(const struct track_node *node);

struct track_node *getNextNode(const struct track_node *currentNode);

struct track_node *getNextSensor(const struct track_node *node);

int distanceBetween(const struct track_node *from, const struct track_node *to);

int getNextClaims(const struct track_node *currentNode, struct SensorClaim *claim);

#endif
