#ifndef __ENGINEER_H
#define __ENGINEER_H
/**
An engineer is a program that operates a train, responsible for train speed.

Workflow:
1. Wait for instruction to reach a given speed at a given time/distance.
2. Figure out the sequence of speed instructions needed to achieve this.

A train moves at a maximum velocity of 50 cm/s, which is 500 000 um/s,
which is 500 um/ms.
We have defined 1 tick = 10 ms, thus 5000 um/10 ms = 5000 um/tick.

The engineer is spawned by the controller, upon a parser command telling the
controller to spawn an engineer for a particular train, via "e train_number".
*/

void engineerCreate(int train_number);

#endif
