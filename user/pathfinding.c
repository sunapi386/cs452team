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
E.g. (path1) C12 -> MR14 <- MR11 -> C13 ->. The reversing at MR14 actually
requires reservation of A3/A4 sensor track_node because the distance from
MR14 to A3 is just 6 cm (shorter than a train's length).
So path1 is expanded to (path2):
[C12,43] ->  [MR14,107] ->  [A4,3] <-  [MR11,101] ->  [C13,44] ->.
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

static void memsetEnstruction(Enstruction *dst, Enstruction *src) {
    if (! dst || ! src) return;
    dst->id = src->id;
    dst->length = src->length;
    dst->togo.node = src->togo.node;
    dst->togo.offset = src->togo.offset;
    memcpy(dst->tracknodes, src->tracknodes, MAX_PATH_LENGTH);
    memcpy(dst->turnops, src->turnops, MAX_PATH_LENGTH);
}

/**
Returns switch number and direction if tn is a branch, in a single int.
Otherwise 0.
*/
static inline int lookahead(track_node *curn, track_node *next) {
    if (! curn || ! next || curn->type != NODE_BRANCH)
        return 0;
    if (next == curn->edge[DIR_STRAIGHT].dest)
        return (curn->idx << 1) | 0;
    if (next == curn->edge[DIR_CURVED].dest)
        return (curn->idx << 1) | 1;
    return 0;
}

/**
Takes a path buffer and converts it into a Ebook, which is a series
of engineer instructions (enstructions).
Returns the number of enstructions, 0 to MAX_EBOOK_LENGTH.
Or -1 on error.
E.g. path2 [C12,43] ->  [MR14,107] ->  [A4,3] <-  [MR11,101] ->  [C13,44] ->
creates the following enstructions:
1/ Togo MR14. Switch false.
2/ Togo C13. Switch false.
*/
int makeEbook(PathBuffer *pb, Ebook *book) {
    if (! pb || ! book || pb->length < 2) return -1;
    memset(book->enstructs, 0, MAX_EBOOK_LENGTH);
    book->length = 0;
    int en_idx = 0;

    {
    track_node *first = pb->tracknodes[0];
    int la = lookahead(first, pb->tracknodes[1]);
    Enstruction ens = {
        .id = en_idx++,
        .togo = {first, 0},
        .tracknodes[0] = pb->tracknodes[0],
        .turnops[0] = la,
        .length = 1,
    };
    Enstruction *newEnst = &book->enstructs[book->length++];
    memsetEnstruction(newEnst, &ens);
    }

    /**
    Follow the path from start to each reverse.
    If there is a reverse, create a new enstruction.
    Otherwise add the current tracknode to the current enstruction.
    */
    for (int i = 1; i < pb->length - 1; i++) {
        int la = lookahead(pb->tracknodes[i], pb->tracknodes[i+1]);
        if (pb->reverse[i]) {
            /**
            Reverse happens here. Make a new enstruction, set it in.
            The last togo should go to here.
            */
            Enstruction *curEnst = &book->enstructs[book->length - 1];
            curEnst->togo.node = pb->tracknodes[i];
            curEnst->togo.offset = 0;
            Enstruction ens = {
                .id = en_idx++,
                .togo = {0, 0},
                .tracknodes[0] = pb->tracknodes[i],
                .turnops[0] = la,
                .length = 1
            };
            Enstruction *newEnst = &book->enstructs[book->length++];
            memsetEnstruction(newEnst, &ens);
        }
        else {
            /**
            No reverse, add current node and branch to the current enstruction.
            */
            Enstruction *curEnst = &book->enstructs[book->length - 1];
            curEnst->tracknodes[curEnst->length] = pb->tracknodes[i];
            curEnst->turnops[curEnst->length++] = la;
        }
    }

    {
    track_node *lasttn = pb->tracknodes[pb->length - 1];
    Enstruction *curEnst = &book->enstructs[book->length - 1];
    curEnst->togo.node = lasttn;
    curEnst->togo.offset = 0;
    curEnst->tracknodes[curEnst->length] = lasttn;
    curEnst->turnops[curEnst->length++] = 0;
    }

    return book->length;
}

void printEnstruction(Enstruction *en) {
    printf(COM2, "Enstruction %d: ", en->id);
    printf(COM2, "Togo %s offset %d length %d ",
        en->togo.node->name, en->togo.offset, en->length);
    for (int i = 0; i < en->length; i++) {
        printf(COM2, "[%s op %d %d] ",
            en->tracknodes[i]->name,
            en->turnops[i] & (-1 << 1),
            en->turnops[i] & 1);
    }
    printf(COM2, "\n");
}

void printEbook(Ebook *book) {
    printf(COM2, "Ebook (%d):\n", book->length);
    for (int i = 0; i < book->length; i++) {
        printEnstruction(&book->enstructs[i]);
    }
    printf(COM2, "\n");
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
