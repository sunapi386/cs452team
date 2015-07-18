#ifndef __SENSOR_H
#define __SENSOR_H

#define MESSAGE_SENSOR_WORKER  11
#define MESSAGE_SENSOR_COURIER 12

// sensorWorker -> sensorServer
typedef struct SensorMessage {
    char data;  // actual data from com1
    char seq;   // sequence number [0,2*NUM_SENSORS-1]
    int time;   // timestamp in ticks
} SensorMessage;

// sensorServer -> sensorCourier
typedef struct SensorUpdate {
    int sensor;
    int time;
} SensorUpdate;

// sensorCourier ->
typedef struct {
    int primaryClaim;
    int secondaryClaim;
} SensorClaim;

typedef struct SensorRequest {
    char type;
    union {
        SensorMessage sm;
        SensorUpdate su;
        SensorClaim sc;
    } data;
} SensorRequest;

/**
Sensor receives sensor data from tracks and handles drawning on the screen.
Also handles sensor manipulations, i.e. stopping a train when it has reached
a certain sensor.
*/
struct SensorData; // defined in trackserver.h

void initSensor();
// displays the time taken to tigger sensor1 then sensor2
void sensorTime(struct SensorData *sensor1, struct SensorData *sensor2);
// e.g. sensorHalt(42, 'a', 3);
void sensorHalt(int train_number, char sensor_group, int sensor_number);
void drawTrackLayoutGraph(char which_track);

#endif
