#ifndef __TRACKSERVER_H
#define __TRACKSERVER_H
#include <user/track_data.h>
/**
Collision Avoidance
1.  Train stay on its own track.
1a. Trains operate track, i.e. the turnout it owns.
2.  One at a time.
3.  Leapfrog problem solved by all-or-nothing request handling.
4.  Giveback when trains finish with track.
*/

typedef struct SensorData {
    char group;
    char offset;
} SensorData;


typedef struct {
    int turnout_number;
    enum {Curved, Straight} direction;
} TurnoutData;

/**
Engineer requests a reservation by Send'ing a TrackServerMessage, and
the TrackServer replies 0 or 1 whether it was successful.
Engineer can have at most MAX_RESERVATION track reserved before trackserver
complains about engineer not releasing track.
On each reservation request, the number of track to reserve is indicated
by num_requested.
*/
#define MAX_RESERVE_SIZE 16
#define MAX_RELEASE_SIZE 16

typedef struct TrackServerMessage {
    int numReserve;
    int numRelease;
    track_node *reserveNodes[MAX_RESERVE_SIZE];
    track_node *releaseNodes[MAX_RELEASE_SIZE];
} TrackServerMessage;

typedef enum TrackServerReply {
    Success = 0,
    ReserveFailSameDir,
    ReserveFailOppositeDir
} TrackServerReply;

void drawTrackLayoutGraph(char track);
void loadTrackStructure(char which_track);
void initTrackServer();

#endif  //__TRACKSERVER_H
