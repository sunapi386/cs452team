#include "track_data.h"
#include "heap.h"
#include <string.h> // memset
#include <stdio.h>
#include <stdlib.h>
typedef enum {false = 0, true = 1} bool;
track_edge *getNextEdge(track_node *node) {
    if(node->type == NODE_BRANCH) {
        return &node->edge[0];
        // return &node->edge[turnoutIsCurved(node->num)];
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

track_node *getPrevOwnedNode(track_node *node, int tid)
{
    // uassert(node);

    if (node->type == NODE_EXIT) return 0;

    track_node* reverseNode = node->reverse;

    if (reverseNode->type == NODE_SENSOR ||
        reverseNode->type == NODE_MERGE)
    {
        track_node* reverseNext = getNextNode(reverseNode);
        return reverseNext->reverse;

    }
    else if (reverseNode->type == NODE_BRANCH)
    {
        track_edge *straightEdge = &reverseNode->edge[DIR_STRAIGHT];
        track_edge *curvedEdge = &reverseNode->edge[DIR_CURVED];

        if (straightEdge->dest->owner == tid)
        {
            return straightEdge->dest->reverse;
        }
        else if (curvedEdge->dest->owner == tid)
        {
            return curvedEdge->dest->reverse;
        }
    }
    return 0;

}



// compile with: gcc track_data.c test_reservation.c -o a.out
int main(int argc, char const *argv[]) {
    printf("Loaded track A\n");
    init_tracka(g_track);

    int my_tid = 63;

    track_node *prevNode = getPrevOwnedNode(&g_track[40], my_tid);
    printf("should be null %d\n", (unsigned)prevNode);

    // own some nodes
    g_track[41].owner = my_tid;
    g_track[111].owner = my_tid;
    prevNode = getPrevOwnedNode(&g_track[40], my_tid);
    printf("should %d == %d\n",
        (unsigned)&g_track[111],
        (unsigned)prevNode);

    return 0;
}
