#include <user/reservation.h>
#include <user/pathfinding.h>
#include <user/turnout.h>
#include <debug.h>

// if the next node is a sensor, then that node is the primary;
// keep going on the "right" track until finding the next sensor

// if the next node is a switch, walk two paths:
// first one is the "right" direction, keep walking to find the next sensor,
// and make that as primary. Then walk down the "wrong" direction, keep walking
// to get the next sensor. that sensor is the secondary claim.

int getNextClaims(struct track_node *prevNode, struct sensorclaim *claim)
{
    uassert(prevNode && claim);

    // given a track node, walk until encounter the next sensor/switch
    track_node *node = prevNode;
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
