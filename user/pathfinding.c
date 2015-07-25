#include <debug.h>
#include <user/sensor.h>
#include <user/turnout.h>
#include <user/track_data.h>
#include <user/pathfinding.h>

track_edge *getNextEdge(const track_node *node)
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

track_node *getNextNode(const track_node *currentNode) {
    track_edge *next_edge = getNextEdge(currentNode);
    return (next_edge == 0 ? 0 : next_edge->dest);
}

track_node *getNextSensor(const track_node *node)
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
int distanceBetween(const track_node *from, const track_node *to)
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
    // unable to find the to node
    return -1;
}

// if the next node is a sensor, then that node is the primary;
// keep going on the "right" track until finding the next sensor

// if the next node is a switch, walk two paths:
// first one is the "right" direction, keep walking to find the next sensor,
// and make that as primary. Then walk down the "wrong" direction, keep walking
// to get the next sensor. that sensor is the secondary claim.

int getNextClaims(const struct track_node *prevNode, struct SensorClaim *claim)
{
    uassert(prevNode && claim);

    // given a track node, walk until encounter the next sensor/switch
    const track_node *node = prevNode;
    for (;;)
    {
        node = getNextNode(node);

        if (node == 0)
        {
            uassert(0);
            return -1;
        }

        switch (node->type)
        {
        case NODE_BRANCH:
        {
            // get the two nodes
            track_node *primary = getNextNode(node);
            uassert(primary);

            track_node *secondary = 0;
            int dir = turnoutIsCurved(node->num) ? DIR_STRAIGHT : DIR_CURVED;
            track_edge *secondaryEdge = &(node->edge[dir]);
            if (secondaryEdge)
            {
                secondary = secondaryEdge->dest;
            }

            // process primary
            if (primary && primary->type != NODE_SENSOR)
            {
                primary = getNextSensor(primary);
                uassert(primary);
            }

            claim->primary = primary->idx;

            // process secondary
            if (secondary)
            {
                if (secondary->type != NODE_SENSOR)
                {
                    secondary = getNextSensor(secondary);
                    uassert(secondary);
                }
                claim->secondary = secondary->idx;
            }
            else
            {
                claim->secondary = -1;
            }

            return 0;
        }
        case NODE_SENSOR:
        {
            claim->primary = node->idx;

            // get the next sensor as the secondary
            track_node *secondary = getNextSensor(node);
            if (secondary != 0) claim->secondary = secondary->idx;
            else claim->secondary = -1;
            return 0;
        }
        case NODE_EXIT:
            claim->primary = -1;
            claim->secondary = -1;
            return 0;
        case NODE_MERGE:
            continue;
        case NODE_ENTER:
        default:
            uassert(0);
            return -1;
        }
    }

    uassert(0);
    return -1;
}
