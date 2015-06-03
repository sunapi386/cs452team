#ifndef __INTERRUPTS_H
#define __INTERRUPTS_H

#include <kernel/task.h>

#define IRQ 0

void initInterrupts();
// puts a task onto interrupt table to wait for interruptID
int awaitInterrupt(int interruptID);
// reschedules tasks that were waiting on interrupts
void handleInterrupt(TaskDescriptor *active);

#endif
