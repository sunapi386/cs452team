#ifndef __CONTEXT_SWITCH_H
#define __CONTEXT_SWITCH_H

struct TaskDescriptor;
void KernelExit(struct TaskDescriptor *task);
void KernelEnter();
void IRQEnter();

#endif
