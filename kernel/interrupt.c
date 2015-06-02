#include <kernel/interrupt.h>
#include <kernel/context_switch.h>
#include <bwio.h>

void initInterrupts()
{
    // software interrupt
    *(unsigned int *)(0x28) = (unsigned int)(&KernelEnter);

    // hardware interrupt
    *(unsigned int *)(0x38) = (unsigned int)(&IRQEnter);

    // select pl190 irq mode
    setICU(VIC1, INT_SELECT, 0);
    setICU(VIC2, INT_SELECT, 0);

    // enable pl190
    setICU(VIC1, INT_ENABLE, 1);
    setICU(VIC2, INT_ENABLE, 1);

    // clear soft int
    setICU(VIC1, SOFT_INT_CLEAR, 1);
    setICU(VIC2, SOFT_INT_CLEAR, 1);
}

void resetInterrupts()
{
    setICU(VIC1, INT_ENABLE, 0);
    setICU(VIC2, INT_ENABLE, 0);
}

