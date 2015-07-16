#include <debug.h>
#include <user/turnout.h>
#include <user/track_data.h>
#include <user/pathfinding.h>

track_edge *getNextEdge(track_node *node)
{
    if(node->type == NODE_BRANCH)
    {
        return &node->edge[turnoutIsCurved(node->num)];
    }
    else if (node->type == NODE_EXIT)
    {
        return 0;
    }
    return &node->edge[DIR_AHEAD];
}

track_node *getNextNode(track_node *currentNode) {
    track_edge *next_edge = getNextEdge(currentNode);
    return (next_edge == 0 ? 0 : next_edge->dest);
}

track_node *getNextSensor(track_node *node)
{
    uassert(node != 0);
    for (;;)
    {
        node = getNextNode(node);
        if (node == 0 || node->type == NODE_SENSOR) return node;
    }
    return 0;
}

// returns positive distance between two nodes in micrometers
// or -1 if the landmark is not found
int distanceBetween(track_node *from, track_node *to)
{
    uassert(from != 0);
    uassert(to != 0);

    int totalDistance = 0;
    track_node *nextNode = from;
    for(int i = 0; i < TRACK_MAX / 2; i++)
    {
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
