#include <pl190.h>
#include <interrupt.h>
#include <context_switch.h>
#include <bwio.h>

void initInterrupt()
{
    // software interrupt
    *(unsigned int *)(0x28) = (unsigned int)(&KernelEnter);

    // hardware interrupt
    *(unsigned int *)(0x38) = (unsigned int)(&IRQEnter);

    // select pl190 irq mode
    *(volatile unsigned int *)(VIC1_BASE + INT_SELECT) = 0;
    *(volatile unsigned int *)(VIC2_BASE + INT_SELECT) = 0;

    // enable pl190
    *(volatile unsigned int *)(VIC1_BASE + INT_ENABLE) = 1;
    *(volatile unsigned int *)(VIC2_BASE + INT_ENABLE) = 1;

    // clear soft int
    *(int *)(VIC1_BASE + SOFT_INT_CLEAR) = 1;
}

void cleanUp()
{
    *(volatile unsigned int *)(VIC1_BASE + INT_ENABLE) = 0;
    *(volatile unsigned int *)(VIC2_BASE + INT_ENABLE) = 0;
}
