#ifndef __ENGINEER_H
#define __ENGINEER_H
/**
An engineer is a program that operates a train, responsible for train speed.

Workflow:
1. Wait for instruction to reach a given speed at a given time/distance.
2. Figure out the sequence of speed instructions needed to achieve this.
3. Enqueue them and use the clockwaiter to delay until the appropriate times.

A train moves at a maximum velocity of 50 cm/s, which is 500 000 um/s,
which is 500 um/ms.
We have defined 1 tick = 10 ms, thus 5000 um/10 ms = 5000 um/tick.


*/

void engineerCreate(int train_number);

#endif
