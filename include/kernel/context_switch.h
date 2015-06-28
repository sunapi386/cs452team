#ifndef __CONTEXT_SWITCH_H
#define __CONTEXT_SWITCH_H

struct TaskDescriptor;

struct Syscall * kernelExit(struct TaskDescriptor *task);
void kernelEnter();
void irqEnter();

#endif
