#ifndef __ATTRIBUTION_H
#define __ATTRIBUTION_H

typedef struct Sensorclaim {
    int primary;
    int secondary;
} SensorClaim;

struct track_node;

int getNextClaims(struct track_node *prevNode, struct Sensorclaim *claim);

#endif
