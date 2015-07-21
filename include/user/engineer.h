#ifndef __ENGINEER_H
#define __ENGINEER_H

typedef struct {
    enum {
        updateSensor,
        updateLocation,
        xMark,
        setSpeed,
        setReverse,
        commandWorkerRequest,
        commandWorkerSpeedSet,
        commandWorkerReverseSet,
        go,
    } type;
    union {
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
            int train_num;
            int node_num;
        } go;
    } data;
} EngineerMessage;


void initEngineer();
void engineerPleaseManThisTrain(); // returns id of engineer task
void engineerReverse();
void engineerSpeedUpdate(int speed); // for now just use 1 train
void engineerLoadTrackStructure(char which_track);
void engineerXMarksTheSpot(int nodeNumber, int offset);
void engineerGo(int train_num, int node_num);

#endif
