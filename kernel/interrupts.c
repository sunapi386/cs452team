#include <utils.h>
#include <debug.h>
#include <kernel/task.h>
#include <kernel/timer.h>
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
    return *(volatile unsigned int *)(base + offset);
}
static inline void enable(unsigned int vicID, unsigned int offset) {
    setICU(vic[vicID], VIC_INT_ENABLE, offset);
}
static inline void clear(int vicID, unsigned int offset) {
    setICU(vic[vicID], VIC_INT_CLEAR, offset);
}


// stacktrace takes a memory address and then decreases this address
// until it detects the bit-pattern of a function name and we’re done.
static void stacktrace() {
    unsigned int *lr;
    asm volatile("mov %0, lr\n\t" : "=r"(lr));
    bwprintf(COM2, "stacktrace from 0x%8x\n\r", lr);
    unsigned int *pc = lr;
    for( /* */ ; (pc[0] & INT_POKE_MASK) != INT_POKE_MASK ; pc-- );
    char *fn_name = (char *) pc - (pc[0] & (~INT_POKE_MASK));
    bwprintf(COM2, "stacktrace function name: %s() 0x%8x\n\r", fn_name, pc);
}

static void undef_instr() {
    bwprintf(COM2, "undef_instr\n\r");
    stacktrace();
}
static void abort_data() {
    bwprintf(COM2, "abort_data\n\r");
    stacktrace();
}

static void abort_prefetch() {
    bwprintf(COM2, "abort_prefetch\n\r");
    stacktrace();
}

void initInterrupts() {
    *(unsigned int *)(0x24) = (unsigned int) &undef_instr;      // undef_instr
    *(unsigned int *)(0x28) = (unsigned int)(&KernelEnter);     // soft int
    *(unsigned int *)(0x2c) = (unsigned int) &abort_prefetch;   // abort_prefetch
    *(unsigned int *)(0x30) = (unsigned int) &abort_data;       // abort_data
    *(unsigned int *)(0x38) = (unsigned int)(&IRQEnter);        // hard int
    vic[0] = VIC1;
    vic[1] = VIC2;
    for(int i = 0; i < 64; i++) {
        interruptTable[i] = 0;
    }
    for(int i = 0; i < 2; i++) {
        setICU(vic[i], VIC_INT_SELECT, 0);      // select pl190 irq mode
        setICU(vic[i], VIC_SOFT_INT_CLEAR, 1);  // clear soft int
    }
    enable(0, 1);       // enable software interrupt
    enable(1, 1 << 19); // enable timer 3
}

int awaitInterrupt(TaskDescriptor *active, int interruptID) {
    if(interruptID < 0 || interruptID > 63) {
        return -1;
    }
    interruptTable[interruptID] = active; // FIXME: 1+ task waiting interruptID?
    active->status = event_blocked;
    return 0; // FIXME
}

void handleInterrupt() { // kernel calls into here
    int statusMask = getICU(VIC2, VIC_IRQ_STATUS);
    if (statusMask & (1 << 19)) {
        // Clear timer interrupt in ICU
        clear(1, 1 << 19);

        // Clear timer interrupt in timer
        clearTimerInterrupt();

        // Queue task if there's a task waiting
        if (interruptTable[51] != 0) {
            queueTask(interruptTable[51]);
            interruptTable[51] = 0;
        }
    }
    /*
    for(int i = 0; i < 2; i++) {
        int statusMask;
        while((statusMask = getICU(vic[i], VIC_IRQ_STATUS))) {
            int interruptOffset = countLeadingZeroes(statusMask);
            queueTask(interruptTable[interruptOffset + 32 * i]);
            clear(i, interruptOffset);
        }
    }*/
}




void stackdump() {
    // dump all registers

}

void undefinedInstructionTesterTask() {
    debug("before");
    asm volatile( "#0xffffffff\n\t" );
    debug("after");
}
