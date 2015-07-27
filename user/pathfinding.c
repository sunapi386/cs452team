#include <user/pathfinding.h>
#include <debug.h>
#include <user/turnout.h>
#include <utils.h>
#include <heap.h>
#include <user/syscall.h>

typedef struct PathNode {
    struct PathNode *from;
    track_node *tn;
    int cost;
    int length;
    bool reverse;
} PathNode;

static void pbinit(PathBuffer *pb) {
    memset(pb->tracknodes, 0, MAX_PATH_LENGTH);
    memset(pb->reverse, false, MAX_PATH_LENGTH);
    pb->length = 0;
    pb->train_num = 0;
}

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
    uassert(src);
    uassert(dst);
    uassert(pb);
    memset(pb->tracknodes, 0, MAX_PATH_LENGTH);
    pb->length = 0;
    memset(g_nodes, 0, EXPLORE_SIZE);
    int num_nodes = 0;
    /**
    No path if src and dst are the same node, reverse.
    */
    if (src->reverse->idx == dst->idx) {
        printf(COM2, "Source and destination node are same.\n");
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
        uassert(num_nodes < EXPLORE_SIZE);

        PathNode *popd = PQHeapPop(&pq);
        if (! popd) {
            /**
            Null pointer was popped, so no path.
            */
            pb->length = -1;
            return -1;
        }

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
            Reached a node not owned by the current train.
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
            Consider both going ahead and reverse.
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
        uassert(path_length < MAX_PATH_LENGTH);
        pb->reverse[path_length] = at->reverse;
        pb->tracknodes[path_length++] = at->tn;
        at = at->from;
    };
    pb->length = path_length;

    printf(COM2, "Searched %d nodes.\n", num_nodes);

    return path_length;
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

void printPath(PathBuffer *pb) {
    String s;
    sinit(&s);
    sputstr(&s, "Path:\n\r");
    for (int i = 0; i < pb->length; i++) {
        sputstr(&s, "[");
        sputstr(&s, pb->tracknodes[i]->name);
        sputstr(&s, ",");
        sputint(&s, pb->tracknodes[i]->idx, 10);
        sputstr(&s, "] ");
        sputstr(&s, pb->reverse[i] ? "<- " : "-> ");
    }
    sputstr(&s, "\n\r");
    PutString(COM2, &s);
}

track_edge *getNextEdge(track_node *node) {
    if(node->type == NODE_BRANCH) {
        return &node->edge[turnoutIsCurved(node->num)];
    }
    else if (node->type == NODE_EXIT) {
        return 0;
    }
    return &node->edge[DIR_AHEAD];
}

track_node *getNextNode(track_node *currentNode) {
    track_edge *next_edge = getNextEdge(currentNode);
    return (next_edge == 0 ? 0 : next_edge->dest);
}

track_node *getNextSensor(track_node *node) {
    uassert(node != 0);
    for (;;) {
        node = getNextNode(node);
        if (node == 0 || node->type == NODE_SENSOR) return node;
    }
    return 0;
}

// returns positive distance between two nodes in micrometers
// or -1 if the landmark is not found
int distanceBetween(track_node *from, track_node *to) {
    uassert(from != 0);
    uassert(to != 0);

    int totalDistance = 0;
    track_node *nextNode = from;
    for(int i = 0; i < TRACK_MAX / 2; i++) {
        // get next edge & node
        track_edge *nextEdge = getNextEdge(nextNode);
        nextNode = getNextNode(nextNode);
        if (nextEdge == 0 || nextNode == 0) break;

        totalDistance += nextEdge->dist;

        if (nextNode == to) return 1000 * totalDistance;
    }
    // unable to find the
    return -1;
}
