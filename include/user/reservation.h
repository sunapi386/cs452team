#ifndef __RESERVATION_H
#define __RESERVATION_H
#include <user/track_data.h>
#include <user/sensor.h>

/**
Handling of reservation calls.
*/

int getNextClaims(struct track_node *currentNode, struct sensorclaim *claim);
#endif
