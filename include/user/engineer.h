#ifndef __ENGINEER_H
#define __ENGINEER_H

#define MAX_NUM_ENGINEER 2

typedef struct {
    enum {
        /* engineer courier */
        updateSensor,
        timeoutSensor,
        
        /* location worker */
        updateLocation,

        /* parser */
        xMark,
        setSpeed,
        setReverse,
        initialize,

        /* command worker */
        commandWorkerRequest,
        commandWorkerSpeedSet,
        commandWorkerReverseSet,
        
        /* sensor courier */
        sensorCourierRequest
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
