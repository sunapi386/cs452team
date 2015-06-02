#ifndef __TIMER_H
#define __TIMER_H

void InitTimer(volatile unsigned int **count);
void UpdateTimer(unsigned int *minutes, unsigned int *seconds, unsigned int *ticks);

#endif
