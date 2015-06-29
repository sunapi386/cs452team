#ifndef __TRACK_CONTROLLER_H
#define __TRACK_CONTROLLER_H

// list of data formats that are accepted by the track controller

typedef struct {
    char group;
    int offset;
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
} ControllerData;


// Called by sensor task to update the controller on latest fired sensor
void sensorTrigger(char group, int offset);

// track controller server
void trackControllerTask();

void initController();

#endif  //__TRACK_CONTROLLER_H
