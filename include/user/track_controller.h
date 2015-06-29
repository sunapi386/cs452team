#ifndef __TRACK_CONTROLLER_H
#define __TRACK_CONTROLLER_H

// list of data formats that are accepted by the track controller

typedef enum {
    tag_sensor = 9,
} TrackDataTag;

typedef struct {
    char sensor;
    char number;
} SensorData;

typedef struct {
    TrackDataTag tag;
    union {
        SensorData data;
    };
} ControllerData;

// track controller server
void trackController();

#endif  //__TRACK_CONTROLLER_H
