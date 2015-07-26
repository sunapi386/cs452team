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
        go,
        xMark,
        setSpeed,
        setReverse,
        initialize,

        /* command worker */
        commandWorkerRequest,
        commandWorkerSpeedSet,
        commandWorkerReverseSet,

        /* sensor courier */
        sensorCourierRequest,
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
        struct {
            int index;
        } go;
    } data;
} EngineerMessage;


void engineerServer();
void engineerGo(int train_num, int node_num);

#endif
