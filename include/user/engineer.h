#ifndef __ENGINEER_H
#define __ENGINEER_H

#define MAX_NUM_ENGINEER 2

typedef struct {
    enum {
        initialize,
        initializeSensorCourier,
        updateSensor,
        updateLocation,
        xMark,
        setSpeed,
        setReverse,
        commandWorkerRequest,
        commandWorkerSpeedSet,
        commandWorkerReverseSet,
    } type;
    union {
        struct {
            int trainNumber;
            int sensorIndex;
        } initialize;
        struct {
            int sensor;
            int time;
        } updateSensor;
        struct {
            int index;
            int offset;
        } xMark;
        struct {
            int speed;
        } setSpeed;
    } data;
} EngineerMessage;

void engineerServer();

#endif
