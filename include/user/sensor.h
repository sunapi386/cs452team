#ifndef __SENSOR_H
#define __SENSOR_H

typedef struct SensorClaim {
    int primary;
    int secondary;
} SensorClaim;

typedef struct SensorRequest {
    enum {
        newSensor,     // sensor worker: A new sensor has been triggered
        initialClaim,  // sensor worker: here is the first sensor that the engineer will hit
        claimSensor,   // sensor courier: I have a new claim to make for my engineer
        requestSensor, // engineer courier: do you have a new delivery for me?
        //invalidateSensor ?
    } type;
    union {
        struct {
            char data;
            char seq;
            int time;
        } newSensor;

        struct {
            int index;
            int engineerTid;
        } initialClaim;

        SensorClaim claimSensor;

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
