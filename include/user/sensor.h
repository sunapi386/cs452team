#ifndef __SENSOR_H
#define __SENSOR_H

#define MESSAGE_SENSOR_COURIER      11
#define MESSAGE_ENGINEER_COURIER    12

// sensorCourier -> sensorServer
typedef struct SensorMessage {
    char data;  // actual data from com1
    char seq;   // sequence number [0,2*NUM_SENSORS-1]
    int time;   // timestamp in ticks
} SensorMessage;

// sensorServer -> engineer
typedef struct SensorUpdate {
    int sensor;
    int time;
} SensorUpdate;

typedef struct SensorRequest {
    char type;
    union {
        SensorMessage sm;
        SensorUpdate su;
    } data;
} SensorRequest;

/**
Sensor receives sensor data from tracks and handles drawning on the screen.
Also handles sensor manipulations, i.e. stopping a train when it has reached
a certain sensor.
*/
struct SensorData; // defined in trackserver.h

// changed back to int sensor;
// note sending index_to_train_data_offset has no way to solve back group & offset
// index = 16 * group + offset - 1;
// to get back group and offset: index + 1 = 16 * group + offset, no way to solve
typedef struct MessageToEngineer {
    int sensor; // (group << 8) & offset
    int time;
} MessageToEngineer;

void initSensor();
// displays the time taken to tigger sensor1 then sensor2
void sensorTime(struct SensorData *sensor1, struct SensorData *sensor2);
// e.g. sensorHalt(42, 'a', 3);
void sensorHalt(int train_number, char sensor_group, int sensor_number);
typedef enum {A, B, None} Track;
void drawTrackLayoutGraph(Track which_track);

#endif
