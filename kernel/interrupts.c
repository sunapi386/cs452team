#include <utils.h>
#include <debug.h>
#include <kernel/task.h>
#include <kernel/uart.h>
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

static inline void save() {
    asm volatile(
        "stmfd sp!, {r0-r12}\n\t"   // store r0-r12 to stack
        "msr cpsr_c, #0xd0\n\t"
        "mov r0, sp\n\t"            // r0 <- sp
        "msr cpsr_c, #0xd3\n\t"
        "stmfd sp!, {r0}\n\t"       // store sp to stack
    );
}

static inline void dump() {
    bwprintf(COM2, "\r\nREGISTERS\r\n");
    for(int i = 13; i >= 0; i--) {
        bwprintf(COM2, "r%d:\t", i);
        asm volatile(
            "mov r0, #1\n\t"
            "ldmfd sp!, {r1}\n\t"
            "bl bwputr\n\t"
            : : : "r0", "r1"
        );
        bwprintf(COM2, "\r\n");
        // register volatile unsigned int reg asm("r3");
        // asm volatile("ldmfd sp!, {r3}");
        // bwprintf(COM2, "->r%d:\t%d\r\n", i, reg);
    }
    bwprintf(COM2, "\r\n");
}

// stacktrace takes a memory address and then decreases this address
// until it detects the bit-pattern of a function name and we’re done.
static inline void stacktrace() {
    save();
    unsigned int *lr;
    asm volatile("mov %0, lr\n\t" : "=r"(lr));
    dump();
    unsigned int *pc = lr;
    while((pc[0] & INT_POKE_MASK) != INT_POKE_MASK) {
        pc--;
    }
    char *fn_name = (char *) pc - (pc[0] & (~INT_POKE_MASK));
    bwprintf(COM2, "STACKTRACE FUNCTION NAME: %s() %x\n\r", fn_name, lr);
}

void undefined_instr() {
    stacktrace();
    bwprintf(COM2, "*\n\r* UNDEFINED INSTRUCTION\n\r*\n\r");
    for(;;); // busy wait do not let kernel go
}

void abort_data() {
    stacktrace();
    bwprintf(COM2, "*\n\r* ABORT DATA\n\r*\n\r");
    for(;;); // busy wait do not let kernel go

}

void abort_prefetch() {
    stacktrace();
    bwprintf(COM2, "*\n\r* ABORT PREFETCH\n\r*\n\r");
    for(;;); // busy wait do not let kernel go

}

void initInterrupts() {
    // ldr  pc, [pc, #0x18] ; 0xe590f018 is the binary encoding
    *(unsigned int *)(0x24) = (unsigned int)(&undefined_instr);  // undef_instr
    *(unsigned int *)(0x28) = (unsigned int)(&kernelEnter);      // soft int
    *(unsigned int *)(0x2c) = (unsigned int)(&abort_prefetch);   // abort_prefetch
    *(unsigned int *)(0x30) = (unsigned int)(&abort_data);       // abort_data
    *(unsigned int *)(0x38) = (unsigned int)(&irqEnter);      // hard int
    vic[0] = VIC1;
    vic[1] = VIC2;
    for(int i = 0; i < 64; i++) {
        interruptTable[i] = 0;
    }
    for(int i = 0; i < 2; i++) {
        setICU(vic[i], VIC_INT_SELECT, 0);      // select pl190 irq mode
    }

    enable(0, 1 << 23); // uart1 recv
    enable(0, 1 << 24); // uart1 xmit
    enable(0, 1 << 25); // uart2 recv
    enable(0, 1 << 26); // uart2 xmit
    enable(1, 1 << 19); // enable timer 3
}

void resetInterrupts() {
    clear(0, 1 << 23); // uart1 recv
    clear(0, 1 << 24); // uart1 xmit
    clear(0, 1 << 25); // uart2 recv
    clear(0, 1 << 26); // uart2 xmit
    clear(1, 1 << 19); // disable timer 3
}

int awaitInterrupt(TaskDescriptor *active, int interruptID) {
    if(interruptID < 0 || interruptID > 63) {
        return -1;
    }
    interruptTable[interruptID] = active; // FIXME: 1+ task waiting interruptID?
    return 0; // FIXME
}

void handleInterrupt() { // kernel calls into here

    int vic1Status = getICU(VIC1, VIC_IRQ_STATUS);
    int vic2Status = getICU(VIC2, VIC_IRQ_STATUS);

    if (vic2Status & (1 << 19)) {
        // Clear timer interrupt in timer
        clearTimerInterrupt();

        // Queue task if there's a task waiting
        TaskDescriptor *td = interruptTable[51];
        if (td != 0) {
            queueTask(td);
            interruptTable[51] = 0;
        }
    }
    else if (vic1Status & (1 << 25))
    {
        // get the character
        char c = getUARTData(COM2);

        bwputc(COM2, c);
        // unblock notifier
    }
    else if (vic1Status & (1 << 26))
    {

    }
}

void stackdump() {
    // dump all registers

}
