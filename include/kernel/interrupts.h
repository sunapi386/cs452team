#ifndef __INTERRUPTS_H
#define __INTERRUPTS_H

#define IRQ 0

struct TaskDescriptor;

void initInterrupts();
// puts a task onto interrupt table to wait for interruptID
int awaitInterrupt(struct TaskDescriptor *active, int interruptID);
// reschedules tasks that were waiting on interrupts
void handleInterrupt();

#endif
