#ifndef __CLOCKSERVER_H
#define __CLOCKSERVER_H

int Time();
int Delay(int ticks);
int DelayUntil(int ticks);

void clockServerTask();

#endif
