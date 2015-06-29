#ifndef __TRACK_CONTROLLER_H
#define __TRACK_CONTROLLER_H

// list of data formats that are accepted by the track controller

#define SENSOR_DATA 9

typedef struct {
    int type;
    union {

    };
} ControllerData;

// track controller server
void trackController();

#endif  //__TRACK_CONTROLLER_H
