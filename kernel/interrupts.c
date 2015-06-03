#include <utils.h>
#include <debug.h>
#include <kernel/task.h>
#include <kernel/pl190.h>
#include <kernel/interrupts.h>
#include <kernel/context_switch.h>

extern void queueTask(struct TaskDescriptor *td);

static int vic[2];  // for iterating
static TaskDescriptor *interruptTable[64]; // 64 interrupt types
static inline void setICU(unsigned int base, unsigned int offset, unsigned int val) {
    *(volatile unsigned int *)(base + offset) = val;
}
static inline unsigned int getICU(unsigned base, unsigned int offset) {
    return *((volatile unsigned int *)base + offset);
}
static inline void enable(unsigned int vicID, unsigned int offset) {
    setICU(vic[vicID], VIC_INT_ENABLE, offset);
}
static inline void clear(int vicID, unsigned int offset) {
    setICU(vic[vicID], VIC_INT_CLEAR, offset);
}

void initInterrupts() {
    *(unsigned int *)(0x28) = (unsigned int)(&KernelEnter); // soft
    *(unsigned int *)(0x38) = (unsigned int)(&IRQEnter);    // hard
    vic[0] = VIC1;
    vic[1] = VIC2;
    for(int i = 0; i < TASK_MAX_TASKS; i++) {
        interruptTable[i] = 0;
    }
    for(int i = 0; i < 2; i++) {
        setICU(vic[i], VIC_INT_SELECT, 0);      // select pl190 irq mode
        setICU(vic[i], VIC_INT_ENABLE, 1);      // enable pl190
        setICU(vic[i], VIC_SOFT_INT_CLEAR, 1);  // clear soft int
    }
}

int awaitInterrupt(TaskDescriptor *active, int interruptID) {
    if(interruptID < 0 || interruptID > 63) {
        return -1;
    }
    interruptTable[interruptID] = active; // FIXME: 1+ task waiting interruptID?
    active->status = event_blocked;
    // set mask
    enable(interruptID / 32, 1 << (interruptID % 32));
    return 0; // FIXME
}

void handleInterrupt() { // kernel calls into here
    debug("got an interrupt");
    for(int i = 0; i < 2; i++) {
        int statusMask;
        while((statusMask = getICU(vic[i], VIC_IRQ_STATUS))) {
            int interruptOffset = countLeadingZeroes(statusMask);
            queueTask(interruptTable[interruptOffset + 32 * i]);
            clear(i, interruptOffset);
        }
    }
}
