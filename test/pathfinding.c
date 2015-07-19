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
    int train_num;
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
#define EXPLORE_SIZE 4096
static PathNode g_nodes[EXPLORE_SIZE];

static inline void setPathNode(PathNode *p, PathNode *f, track_node *t, int c) {
    p->from = f;
    p->tn = t;
    p->cost = c;
}

static inline int nodeCost(PathNode *pn) {
    return pn->cost;
}

DECLARE_HEAP(PQHeap, PathNode*, nodeCost, 12, <);

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
        if (dst == popd->tn) {
            end = popd;
            break;
        }

        if (popd->tn->type == NODE_EXIT) {
            continue;
        }

        if (popd->tn->owner != -1 && popd->tn->owner != pb->train_num) {
            /**
            Make the cost effectively infinite high, so it is not considered.
            77950 is sum of all distances on tracks A & B.
            TODO: but even with cost very high it is possible to get a path.
            */
            popd->cost += 77950;
        }

        if (popd->tn->type == NODE_BRANCH) {
            PathNode *straight = &g_nodes[num_nodes++];
            setPathNode(straight,
                        popd,
                        popd->tn->edge[DIR_STRAIGHT].dest,
                        popd->cost + popd->tn->edge[DIR_STRAIGHT].dist);
            PQHeapPush(&pq, straight);

            PathNode *curved = &g_nodes[num_nodes++];
            setPathNode(curved,
                        popd,
                        popd->tn->edge[DIR_CURVED].dest,
                        popd->cost + popd->tn->edge[DIR_CURVED].dist);

            PQHeapPush(&pq, curved);
        }
        else {
            PathNode *ahead = &g_nodes[num_nodes++];
            setPathNode(ahead,
                        popd,
                        popd->tn->edge[DIR_AHEAD].dest,
                        popd->cost + popd->tn->edge[DIR_AHEAD].dist);
            PQHeapPush(&pq, ahead);
        }
    }

    /**
    We have found a path to dest, accessible via end.
    Construct the path from end to start, filling in PathBuffer.
    */
    PathNode *at = end;
    int path_length = 0;
    while (at != start) {
        // uassert(path_length < MAX_PATH_LENGTH);
        pb->tracknodes[path_length++] = at->tn;
        at = at->from;
    };
    pb->tracknodes[path_length++] = at->tn;
    pb->length = path_length;

    /**
    A path from start to end is easier to read, so we'll reverse in place.
    */
    for (int i = 0, j = path_length - 1; i < j; i++, j--) {
        track_node *c = pb->tracknodes[i];
        pb->tracknodes[i] = pb->tracknodes[j];
        pb->tracknodes[j] = c;
    }

    return path_length;
}

void printPath(PathBuffer *pb) {
    printf("Path:\n");
    for (int i = 0; i < pb->length; i++) {
        printf("-> [%s,%d] ",
            pb->tracknodes[i]->name,
            pb->tracknodes[i]->idx);
    }
    printf("\n");
}

int shortestRoute(track_node *src, track_node *dst, PathBuffer *pathb) {
    PathBuffer pb[4];
    planRoute(src, dst, &pb[0]);
    planRoute(src, dst->reverse, &pb[1]);
    planRoute(src->reverse, dst, &pb[2]);
    planRoute(src->reverse, dst->reverse, &pb[3]);

    PathBuffer *lowest = &pb[0];
    for (int i = 1; i < 4; i++)
        if (lowest->length > pb[i].length)
            lowest = &pb[i];

    memcpy(pathb, lowest, sizeof(PathBuffer));
    return lowest->length;
}

track_node g_track[TRACK_MAX]; // This is guaranteed to be big enough.

typedef enum {false = 0, true = 1} bool;
bool valid_node(int n) {
    return 0 <= n && n <= 144;
}

// compile with: gcc track_data.c pathfinding.c -o a.out
int main(int argc, char const *argv[]) {
    int from, to;
    int blocked = -1;
    int blocked2 = -1;
    switch(argc) {
        case 5:
            blocked2 = atoi(argv[4]);
        case 4:
            blocked = atoi(argv[3]);
        case 3:
            to = atoi(argv[2]);
            from = atoi(argv[1]);
            break;
        default:
            printf("Usage: %s [from to [blocked [blocked]]]", argv[0]);
            return -1;
    }
    if (! (valid_node(from) && valid_node(to))) {
        printf("Bad node number %d %d", from, to);
        return -1;
    }
    init_tracka(g_track);
    printf("Loaded track A\n");
    if (blocked != -1 && valid_node(blocked)) {
        g_track[blocked].owner = 90; /* 90 is a non-existant train number */
    }
    if (blocked2 != -1 && valid_node(blocked2)) {
        g_track[blocked2].owner = 90; /* 90 is a non-existant train number */
    }
    track_node *src = &g_track[from];
    track_node *dst = &g_track[to];
    PathBuffer pb;
    pb.train_num = 66;
    int ret = planRoute(src, dst, &pb);
    // int ret = shortestRoute(src, dst, &pb);
    printPath(&pb);
    printf("%d\n", ret);
    return 0;
}
