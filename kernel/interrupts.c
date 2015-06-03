#include <debug.h>
#include <kernel/task.h>
#include <kernel/interrupts.h>
#include <kernel/context_switch.h>

static int vic[2];  // for iterating
static TaskDescriptor *interruptTable[64]; // 64 interrupt types
static inline void setICU(unsigned int base, unsigned int offset, unsigned int val) {
    *(unsigned int *)(base + offset) = val; // FIXME: volatile for O2?
}

void initInterrupts() {
    *(unsigned int *)(0x28) = (unsigned int)(&KernelEnter); // software interrupt
    *(unsigned int *)(0x38) = (unsigned int)(&IRQEnter);    // hardware interrupt
    vic[0] = VIC1;
    vic[1] = VIC2;
    for(int i = 0; i < TASK_MAX_TASKS; i++) {
        interruptTable[i] = 0;
    }

    for(int i = 0; i < 2; i++) {
        setICU(vic[i], INT_SELECT, 0);      // select pl190 irq mode
        setICU(vic[i], INT_ENABLE, 1);      // enable pl190
        setICU(vic[i], SOFT_INT_CLEAR, 1);  // clear soft int
    }

}

void resetInterrupts(unsigned int vicID) {
    setICU(vic[0], INT_ENABLE, 0);
    setICU(vic[1], INT_ENABLE, 0);
}

int awaitInterrupt(TaskDescriptor *active, int interruptID) {
    if(interruptID < 0 || interruptID > 63) {
        return -1;
    }
    interruptTable[interruptID] = active;
    active->status = event_blocked;
    // FIXME: reset interrupt not need here because kernel does it?
}
void handleInterrupt() {

}
