#include "track_data.h"
#include "heap.h"
#include <string.h> // memset
#include <stdio.h>
#include <stdlib.h>
////
typedef enum {false = 0, true = 1} bool;

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
    bool reverse[MAX_PATH_LENGTH];
} PathBuffer;

////
typedef struct PathNode {
    struct PathNode *from;
    track_node *tn;
    int cost;
    int length;
    bool reverse;
} PathNode;

static int pbcopy(PathBuffer *dst, PathBuffer *src) {
    if (! src || ! dst) {
        return -1;
    }
    dst->train_num = src->train_num;
    memcpy(dst->tracknodes, src->tracknodes, MAX_PATH_LENGTH);
    dst->length = src->length;
    memcpy(dst->reverse, src->reverse, MAX_PATH_LENGTH);
    return dst->length;
}

/**
Because there is no malloc, a large buffer is preallocated for PathNode.
*/
#define EXPLORE_SIZE 4096
static PathNode g_nodes[EXPLORE_SIZE];

static inline
void setPathNode(PathNode *p, PathNode *f, track_node *t, int c, int l, bool r) {
    p->from = f;
    p->tn = t;
    p->cost = c;
    p->length = l;
    p->reverse = r;
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
    No path if src and dst are the same node, reverse.
    */
    if (src->reverse->idx == dst->idx) {
        printf("Source and destination node are same.\n");
        pb->length = 0;
        return 0;
    }

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
    setPathNode(start, 0, src, 0, 0, false);

    PQHeap pq;
    pq.count = 0;

    PQHeapPush(&pq, start);
    while (1) {
        // uassert(num_nodes < EXPLORE_SIZE);

        PathNode *popd = PQHeapPop(&pq);
        if (! popd) {
            /**
            Null pointer was popped, so no path.
            */
            printf("No path\n");
            pb->length = -1;
            return -1;
        }

        // if (popd->from) {
        //     printf("popd from %s, cur %s, cost %d, len %d\n",
        //         popd->from->tn->name, popd->tn->name, popd->cost, popd->length);
        // }

        if (dst == popd->tn) {
            end = popd;
            break;
        }

        if (popd->tn->type == NODE_EXIT || popd->tn->type == NODE_ENTER) {
            /**
            Exit nodes do not lead anywhere.
            */
            PathNode *reverse = &g_nodes[num_nodes++];
            setPathNode(reverse,
                        popd,
                        popd->tn->reverse->edge[DIR_STRAIGHT].dest,
                        popd->cost + popd->tn->reverse->edge[DIR_STRAIGHT].dist,
                        popd->length + 1,
                        true);
            PQHeapPush(&pq, reverse);
            continue;
        }


        if (popd->tn->owner != -1 && popd->tn->owner != pb->train_num) {
            /**
            Reached a node not owned by current train.
            */
            continue;
        }

        if (popd->length > 22) {
            /**
            Stop checking this path early, longest path is
            18 for track A and 22 for track B.
            */
            continue;
        }

        if (popd->tn->type == NODE_BRANCH) {
            /**
            Consider both straight and curved directions.
            */
            PathNode *straight = &g_nodes[num_nodes++];
            setPathNode(straight,
                        popd,
                        popd->tn->edge[DIR_STRAIGHT].dest,
                        popd->cost + popd->tn->edge[DIR_STRAIGHT].dist,
                        popd->length + 1,
                        false);
            PQHeapPush(&pq, straight);

            PathNode *curved = &g_nodes[num_nodes++];
            setPathNode(curved,
                        popd,
                        popd->tn->edge[DIR_CURVED].dest,
                        popd->cost + popd->tn->edge[DIR_CURVED].dist,
                        popd->length + 1,
                        false);

            PQHeapPush(&pq, curved);
        }
        else {
            /**
            Consider both going ahead and reverse, only on non-branch nodes.
            */
            PathNode *ahead = &g_nodes[num_nodes++];
            setPathNode(ahead,
                        popd,
                        popd->tn->edge[DIR_AHEAD].dest,
                        popd->cost + popd->tn->edge[DIR_AHEAD].dist,
                        popd->length + 1,
                        false);
            PQHeapPush(&pq, ahead);

            PathNode *reverse = &g_nodes[num_nodes++];
            setPathNode(reverse,
                        popd,
                        popd->tn->reverse->edge[DIR_AHEAD].dest,
                        popd->cost + popd->tn->reverse->edge[DIR_AHEAD].dist,
                        popd->length + 1,
                        true);
            PQHeapPush(&pq, reverse);
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
        pb->reverse[path_length] = at->reverse;
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

    printf("Searched %d nodes.\n", num_nodes);

    return path_length;
}

void printPath(PathBuffer *pb) {
    printf("Path (%d):\n", pb->length);
    for (int i = 0; i < pb->length; i++) {
        printf("[%s,%d] ", pb->tracknodes[i]->name, pb->tracknodes[i]->idx);
        printf("%s ", pb->reverse[i] ? "<- " : "-> ");
    }
    printf("\n");
}

track_node g_track[TRACK_MAX]; // This is guaranteed to be big enough.

bool valid_node(int n) {
    return 0 <= n && n <= 144;
}

static void pbinit(PathBuffer *pb) {
    memset(pb->tracknodes, 0, MAX_PATH_LENGTH);
    memset(pb->reverse, false, MAX_PATH_LENGTH);
    pb->length = 0;
    pb->train_num = 0;
}

/**
Returns an expanded version of the path, so the engineer can make reservations.
Some reversing paths are too short and need more reservations.
E.g. C12 -> MR14 <- MR11 -> C13. In this case reversing at MR14 actually
requires reservation of A3/A4 sensor track_node because the distance from
MR14 to A3 is just 6 cm (shorter than a train's length).
*/
int expandPath(PathBuffer *pb) {
    if (! pb) {
        return -1;
    }
    PathBuffer clonedpb;
    pbcopy(&clonedpb, pb);
    pbinit(pb);
    /**
    By estimate, a train is approximately 20cm long. Factoring in that we are
    not very accurate in estimating short move distances, we should reserve the
    next two train-lengths from any track node we do a reverse on.
    Then iterate through the train nodes (in the copied PathBuffer):
        If there is a reverse on the tracknode,
            make sure to keep reserving track_nodes until padding is used up.
        Otherwise just copy back the original tracknode.
    */
    const int PADDING = 2 * 20 * 10; /* two train lengths, in mm */
    for (int i = 0; i < clonedpb.length; i++) {
        pb->tracknodes[pb->length++] = clonedpb.tracknodes[i];
        if (clonedpb.reverse[i]) {
            pb->reverse[pb->length] = true; // pbinit initalizes reverse false
            int extra = 0;
            while (extra < PADDING) {
                /**
                planRoute never reverses on branch nodes.
                */
                track_node *next = clonedpb.tracknodes[i]->edge[DIR_AHEAD].dest;
                pb->tracknodes[pb->length++] = next;
                extra += next->edge[DIR_AHEAD].dist;
            }
        }
    }

    return pb->length;
}

// compile with: gcc track_data.c pathfinding.c -o a.out
int main(int argc, char const *argv[]) {
    char track = 0;
    int from = -1;
    int to = -1;
    int blocked = -1;
    int blocked2 = -1;
    switch(argc) {
    case 6:
        blocked2 = atoi(argv[5]);
        if (! valid_node(from)) {
            printf("Bad blocked 2 node %d", blocked2);
            return -1;
        }
    case 5:
        blocked = atoi(argv[4]);
        if (! valid_node(from)) {
            printf("Bad blocked node %d", blocked);
            return -1;
        }
    case 4:
        track = argv[1][0];
        to = atoi(argv[3]);
        from = atoi(argv[2]);
        if (track == 'a') {
            printf("Loaded track A\n");
            init_tracka(g_track);
        }
        else if (track == 'b') {
            printf("Loaded track B\n");
            init_trackb(g_track);
        }
        else {
            printf("Bad track %c", track);
            return -1;
        }
        if (! valid_node(to)) {
            printf("Bad to node %d", to);
            return -1;
        }
        if (! valid_node(from)) {
            printf("Bad from node %d", from);
            return -1;
        }
        break;
    default:
        printf("Usage: %s track from to [blocked [blocked]]", argv[0]);
        return -1;
    }
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
    printPath(&pb);
    printf("expandedPath:\n");
    expandPath(&pb);
    printPath(&pb);
    return 0;
}
