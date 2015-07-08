#ifndef __TRAIN_H
#define __TRAIN_H

void initTrain();
void trainSetSpeed(int train_number, int train_speed);
void trainSetReverse(int train_number);
void trainSetReverseNicely(int train_number);
void trainSetSwitch(int switch_number, char direction); // only use in turnout.c
void trainSetLight(int train_number, int on);

#endif
