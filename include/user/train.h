#ifndef __TRAIN_H
#define __TRAIN_H
#include <utils.h>

void initTrain();
void trainSetSpeed(int train_number, int train_speed);
void trainSetReverse(int train_number);
void trainSetSwitch(int switch_number, bool curved);

#endif
