#ifndef __SENSOR_H
#define __SENSOR_H

#define MESSAGE_SENSOR_WORKER    11
#define MESSAGE_SENSOR_COURIER   12
#define MESSAGE_ENGINEER_COURIER 13

// sensorWorker -> sensorServer
typedef struct WorkerMessage {
    char data;  // actual data from com1
    char seq;   // sequence number [0,2*NUM_SENSORS-1]
    int time;   // timestamp in ticks
} WorkerMessage;

// engineer -> sensorCourier -> sensor server
typedef struct {
    int primaryClaim;
    int secondaryClaim;
} SensorClaim;

typedef struct SensorRequest {
    enum {
        newSensor,     // sensor worker: A new sensor has been triggered
        claimSensor,   // sensor courier: I have a new claim to make for my engineer
        requestSensor, // engineer courier: do you have a new delivery for me?
        //invalidateSensor ?
    } type;
    union {
        WorkerMessage wm; // sensor worker <-> engineer
        SensorClaim sc;   // engineer -> sensor courier - > sensor server
    } data;
} SensorRequest;


// sensorServer -> engineerCourier -> engineer
typedef struct SensorUpdate {
    int sensor;
    int time;
} SensorUpdate;

/**
Sensor receives sensor data from tracks and handles drawning on the screen.
Also handles sensor manipulations, i.e. stopping a train when it has reached
a certain sensor.
*/
struct SensorData; // defined in trackserver.h

void initSensor();
void sensorHalt(int train_number, char sensor_group, int sensor_number);
void drawTrackLayoutGraph(char which_track);

#endif
