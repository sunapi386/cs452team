#ifndef __CONTEXT_SWITCH_H
#define __CONTEXT_SWITCH_H

struct TaskDescriptor;
void kernelExit(struct TaskDescriptor *task);
void kernelEnter() __attribute__ ((interrupt ("SWI")));
void irqEnter() __attribute__ ((interrupt ("IRQ")));

#endif
