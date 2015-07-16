#ifndef __PATHFINDING_H
#define __PATHFINDING_H
#include <user/track_data.h> // track_node decl

struct Position {
    struct track_node *node;
    int offset;
};

void plan_route(struct track_node *start, struct track_node *end);

int planPath(
    struct track_node *list,
    int train_id,
    struct track_node *start,
    struct track_node *goal,
    struct track_node **output,
    int *output_cost
);

// Puts in ret the new position after traveling dist from start,
// given that the switches are in the state given by turnouts.
int alongTrack(
    TurnoutTable turnouts,
    struct Position *start,
    int dist,
    struct Position *end,
    struct track_node **path,
    struct track_node **follow,
    struct TrackEdge **edges,
    int *edgeCount,
    bool beyond
);

// Returns the first node involved in a reverse, or the end if there isn't one.
struct track_node **nextReverse(struct track_node **path, struct track_node *end);

int distance(TurnoutTable turnouts, struct Position *from, struct Position *to);


#endif
