#ifndef __TRACK_CONTROLLER_H
#define __TRACK_CONTROLLER_H

// list of data formats that are accepted by the track controller

typedef struct {
    char sensor;
    char number;
} SensorData;

typedef struct {
    int turnout_number;
    enum {Curved, Straight} direction;
} TurnoutData;

typedef struct {
    enum {
        sensor,
        turnout,
    } type;
    union {
        SensorData sensor;
        TurnoutData turnout;
    } data;
} ControllerData;

// track controller server
void trackController();

#endif  //__TRACK_CONTROLLER_H
