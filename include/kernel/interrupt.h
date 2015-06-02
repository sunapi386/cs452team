#ifndef __INTERRUPT_H
#define __INTERRUPT_H
#include <kernel/pl190.h>

#define IRQ 0

void initInterrupts();
void resetInterrupts();

static inline void setICU(unsigned int base,
                          unsigned int offset,
                          unsigned int val)
{
    *(unsigned int *)(base + offset) = val;
}

#endif
