#include "track_data.h"
#include "heap.h"
#include <string.h> // memset
#include <stdio.h>
#include <stdlib.h>
////

struct Position {
    track_node *node;
    int offset;
};

#define MAX_PATH_LENGTH TRACK_MAX // very generous
/**
PathBuffer for passing around paths.
Array of track_node indecies, ordered from destination (0) to source (length).
*/
typedef struct PathBuffer {
    track_node *tracknodes[MAX_PATH_LENGTH];
    int length;
} PathBuffer;

////
typedef struct PathNode {
    struct PathNode *from;
    track_node *tn;
    int cost;
} PathNode;

/**
Because there is no malloc, a large buffer is preallocated for PathNode.
*/
#define EXPLORE_SIZE 1000
static PathNode g_nodes[EXPLORE_SIZE];

static inline void setPathNode(PathNode *p, PathNode *f, track_node *t, int c) {
    p->from = f;
    p->tn = t;
    p->cost = c;
}

static inline int nodeCost(PathNode *pn) {
    return pn->cost;
}

DECLARE_HEAP(PQHeap, PathNode*, nodeCost, 8, <);

/**
Returns the number of track_node pointers to from src to dst.
*/
int planRoute(track_node *src, track_node *dst, PathBuffer *pb) {
    // uassert(src);
    // uassert(dst);
    // uassert(pb);
    memset(pb->tracknodes, 0, MAX_PATH_LENGTH);
    pb->length = 0;
    memset(g_nodes, 0, EXPLORE_SIZE);
    int num_nodes = 0;

    /**
    At any time in the PQ, it only contains the horizon PathNode.
    The PathNode with the minimum pathcost is popped from the PQ,
    and is followed for one more node, and pushed back onto the PQ.
    If the node followed was a branch, two child PathNode are created,
    one for each possible direction.
    Otherwise PathNode is created for the next track_node.
    */
    PathNode *start = &g_nodes[num_nodes++];
    PathNode *end;
    setPathNode(start, 0, src, 0);

    PQHeap pq;
    pq.count = 0;

    PQHeapPush(&pq, start);
    while (1) {
        // uassert(num_nodes < EXPLORE_SIZE);

        PathNode *popd = PQHeapPop(&pq);
        if (popd->tn->type == NODE_BRANCH) {
            PathNode *straight = &g_nodes[num_nodes++];
            setPathNode(straight,
                        popd,
                        popd->tn->edge[DIR_STRAIGHT].dest,
                        popd->cost + popd->tn->edge[DIR_STRAIGHT].dist);
            if (dst == popd->tn->edge[DIR_STRAIGHT].dest) {
                end = straight;
                break;
            }
            PQHeapPush(&pq, straight);

            PathNode *curved = &g_nodes[num_nodes++];
            setPathNode(curved,
                        popd,
                        popd->tn->edge[DIR_CURVED].dest,
                        popd->cost + popd->tn->edge[DIR_CURVED].dist);

            if (dst == popd->tn->edge[DIR_CURVED].dest) {
                end = curved;
                break;
            }
            PQHeapPush(&pq, curved);
        }
        else {
            PathNode *ahead = &g_nodes[num_nodes++];
            setPathNode(ahead,
                        popd,
                        popd->tn->edge[DIR_AHEAD].dest,
                        popd->cost + popd->tn->edge[DIR_AHEAD].dist);
            if (dst == popd->tn->edge[DIR_AHEAD].dest) {
                end = ahead;
                break;
            }
            PQHeapPush(&pq, ahead);
        }
    }

    /**
    We have found a path to dest, accessible via end.
    Construct the path from end to start, filling in PathBuffer.
    */
    PathNode *at = end;
    int path_length = 0;
    do {
        // uassert(path_length < MAX_PATH_LENGTH);
        pb->tracknodes[path_length++] = at->tn;
        at = at->from;
    } while (at != start);
    pb->length = path_length;

    return path_length;
}

void printPath(PathBuffer *pb) {
    printf("Path:\n");
    for (int i = 0; i < pb->length; i++) {
        printf("-> %s ", pb->tracknodes[i]->name);
    }
    printf("\n");
}

track_node g_track[TRACK_MAX]; // This is guaranteed to be big enough.

// compile with: gcc track_data.c pathfinding.c -o a.out
int main(int argc, char const *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <from node number> <to node number>", argv[0]);
        return -1;
    }
    int from = atoi(argv[1]);
    int to = atoi(argv[2]);
    if (0 <= from && from <= 144 && 0 <= to && to <= 144) {
        printf("Bad node number %d %d", from, to);
        return -1;
    }
    init_tracka(g_track);
    track_node *src = &g_track[from];
    track_node *dst = &g_track[to];
    PathBuffer pb;
    int ret = planRoute(src, dst, &pb);
    printPath(&pb);
    printf("%d\n", ret);
    return 0;
}
