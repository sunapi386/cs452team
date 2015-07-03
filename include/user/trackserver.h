#ifndef __TRACKSERVER_H
#define __TRACKSERVER_H

// list of data formats that are accepted by the track controller
typedef struct SensorData {
    char group;
    char offset;
} SensorData;

typedef struct {
    int turnout_number;
    enum {Curved, Straight} direction;
} TurnoutData;

typedef struct {
    enum {
        type_sensor,
        type_turnout,
    } type;
    union {
        SensorData sensor;
        TurnoutData turnout;
    } data;
} TrackRequest;


// Called by sensor task to update the controller on latest fired sensor
void sensorTrigger(char group, int offset);

void initTrackServer();

#endif  //__TRACKSERVER_H
