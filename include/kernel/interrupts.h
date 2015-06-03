#ifndef __INTERRUPTS_H
#define __INTERRUPTS_H

#define IRQ 0

void initInterrupts();
void resetInterrupts();
// puts a task onto interrupt table to wait for interruptID
int awaitInterrupt(int interruptID);
// reschedules tasks that were waiting on interrupts
void handleInterrupt();

#endif
