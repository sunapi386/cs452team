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
#define MAX_RESERVATION 10
typedef struct TrackServerMessage {
    enum {
        RESERVATION,
        QUIT,
    } type;
    union {
        struct Reservation {
            enum {
                RESERVE,
                RELEASE,
            } op;
            int train_num;
            int num_requested;
            track_node *nodes[MAX_RESERVATION];
        } reservation;
    };
} TrackServerMessage;


// Called by sensor task to update the controller on latest fired sensor
void sensorTrigger(char group, int offset);

void initTrackServer();

#endif  //__TRACKSERVER_H
