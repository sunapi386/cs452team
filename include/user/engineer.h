#ifndef __ENGINEER_H
#define __ENGINEER_H
/**
An engineer is a program that operates a train, responsible for train speed.

Workflow:
1. Wait for instruction to reach a given speed at a given time/distance.
2. Figure out the sequence of speed instructions needed to achieve this.

A train moves at a maximum estimated velocity of 50 cm/s, which is 500 000 um/s,
which is 500 um/ms. We define 10 ticks is 1 ms, thus in terms of ticks,
500 um/ms / (10 ticks / 1 ms) = 5000 um/tick.

The engineer is spawned by the controller, upon a parser command telling the
controller to spawn an engineer for a particular train, via "e train_number".
*/

typedef struct Enstruction {
    int speed;      // how fast we want the train to go
    int time;       // how long to go for
    int distance;   // how far to the target
} Enstruction;

void initEngineer();
int engineerPleaseManThisTrain(); // returns id of engineer task

#endif
