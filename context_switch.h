#ifndef __CONTEXT_SWITCH_H
#define __CONTEXT_SWITCH_H

void KernelExit(volatile struct TaskDescriptor *task, struct Syscall **request);
void KernelEnter();

#endif
