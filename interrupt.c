#include <interrupt.h>

void initInterrupt()
{
    // software interrupt
    *(unsigned int *)(0x28) = (unsigned int)(&KernelEnter);

    // hardware interrupt
    *(unsigned int *)(0x38) = (unsigned int)(&IRQHandler);

    // enable pl190

}
