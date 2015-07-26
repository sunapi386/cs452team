#ifndef __PATHFINDING_H
#define __PATHFINDING_H
#include <user/track_data.h>
#include <utils.h> // bool

struct track_node;
struct track_edge;
struct SensorClaim;

#define MAX_PATH_LENGTH TRACK_MAX // very generous

/**
    PathBuffer for passing around paths.
    Array of track_node indecies, ordered from destination (0) to source (length).
*/
typedef struct PathBuffer {
    int train_num;   /* Engineer: train_number for reservation planning */
    track_node *tracknodes[MAX_PATH_LENGTH];
    int cost;
    int length;
    bool reverse[MAX_PATH_LENGTH];
} PathBuffer;


struct track_edge *getNextEdge(const struct track_node *node);

struct track_node *getNextNode(const struct track_node *currentNode);

struct track_node *getNextSensor(const struct track_node *node);

int distanceBetween(const struct track_node *from, const struct track_node *to);

int getNextClaims(const struct track_node *currentNode, struct SensorClaim *claim);

/**
    planRoute
    Returns -1 if no path exists, otherwise the length of the path.
*/
int planRoute(track_node *start, track_node *end, PathBuffer *pb);
void printPath(PathBuffer *pb);

#endif
