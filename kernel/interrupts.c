#include <utils.h>
#include <debug.h>
#include <ts7200.h>
#include <events.h>
#include <kernel/task.h>
#include <kernel/uart.h>
#include <kernel/timer.h>
#include <kernel/pl190.h>
#include <kernel/interrupts.h>
#include <kernel/context_switch.h>

#define UART1_RECV_MASK 1 << 23 // uart1 recv
#define UART1_XMIT_MASK 1 << 24 // uart1 xmit
#define UART1_OR_MASK   1 << 20 // uart1 OR
#define UART2_RECV_MASK 1 << 25 // uart2 recv
#define UART2_XMIT_MASK 1 << 26 // uart2 xmit
#define TIMER3_MASK     1 << 19 // timer 3

#define OR_MODEM_MASK 1
    #define DCTS_MASK 1
    #define CTSN_MASK 1 << 4
#define OR_RECV_MASK 1 << 1
#define OR_XMIT_MASK 1 << 2

extern void queueTask(struct TaskDescriptor *td);

static TaskDescriptor *eventTable[5]; // 64 interrupt types
static inline void setICU(unsigned int base, unsigned int offset, unsigned int val) {
    *(volatile unsigned int *)(base + offset) = val;
}
static inline unsigned int getICU(unsigned base, unsigned int offset) {
    return *(volatile unsigned int *)(base + offset);
}
static inline void enable(unsigned int vic, unsigned int offset) {
    setICU(vic, VIC_INT_ENABLE, offset);
}
static inline void clear(unsigned int vic, unsigned int offset) {
    setICU(vic, VIC_INT_CLEAR, offset);
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

void initInterrupts()
{
    *(unsigned int *)(0x28) = (unsigned int)(&kernelEnter);      // soft int
    *(unsigned int *)(0x38) = (unsigned int)(&irqEnter);         // hard int

    for(int i = 0; i < 64; i++) {
        eventTable[i] = 0;
    }

    setICU(VIC1, VIC_INT_SELECT, 0);
    setICU(VIC2, VIC_INT_SELECT, 0);

    //enable(VIC1, 1); // soft int
    enable(VIC2, UART1_OR_MASK); // uart1 OR
    enable(VIC1, UART2_RECV_MASK); // uart2 recv
    enable(VIC1, UART2_XMIT_MASK); // uart2 xmit
    enable(VIC2, TIMER3_MASK); // enable timer 3

}

void resetInterrupts() {
    clear(VIC2, UART1_OR_MASK);   // uart1 OR
    clear(VIC1, UART2_RECV_MASK); // uart2 recv
    clear(VIC1, UART2_XMIT_MASK); // uart2 xmit
    clear(VIC2, TIMER3_MASK);     // disable timer 3
}

int awaitInterrupt(TaskDescriptor *active, unsigned int event) {
    if (event > UART2_XMIT_EVENT)
    {
        return -1;
    }

    // Turn on IO interrutpts
    // (selectively turned off in handleInterrupt)
    switch (event) {
    case UART1_XMIT_EVENT:
    case UART1_RECV_EVENT:
    case UART2_XMIT_EVENT:
    case UART2_RECV_EVENT:
        setUARTCtrl(event, 1);
        break;
    default:
        break;
    }
    eventTable[event] = active;
    return 0;
}

void handleInterrupt() {
    static char ctsOn = -1;
    static char xmitRdy = -1;
    int vic1Status = getICU(VIC1, VIC_IRQ_STATUS);
    int vic2Status = getICU(VIC2, VIC_IRQ_STATUS);

    // Timer 3 underflow
    if (vic2Status & TIMER3_MASK) {

        // Clear timer interrupt in timer
        clearTimerInterrupt();

        // Queue task if there's a task waiting
        TaskDescriptor *td = eventTable[TIMER_EVENT];
        if (td != 0)
        {
            taskSetRet(td, 0);
            //td->ret = 0;
            queueTask(td);
            eventTable[TIMER_EVENT] = 0;
        }
    }
    // UART 1 OR
    else if (vic2Status & UART1_OR_MASK)
    {
        // Get interrupt status
        int status = getUARTIntStatus(COM1);

        // UART 1 modem interrupt
        if (status & OR_MODEM_MASK)
        {
            // Get cts
            int modemStatus = getUART1ModemStatus();

            // clear modem interrupt
            clearUART1ModemInterrupt();

            int cts = (modemStatus & CTSN_MASK) != 0;
            int dcts = (modemStatus & DCTS_MASK) != 0;

            if (cts && dcts)
            {
                ctsOn = 1;
            }
            else
            {
                // need to wait for another cts
                ctsOn = 0;
            }

            // Ready to send a byte
            if (ctsOn && xmitRdy)
            {
                // Reset states
                ctsOn = 0;
                xmitRdy = 0;

                // We can safely send a bit;
                // unblock if there is a guy waiting
                TaskDescriptor *td = eventTable[UART1_XMIT_EVENT];
                if (td != 0)
                {
                    //td->ret = (UART1_BASE + UART_DATA_OFFSET);
                    taskSetRet(td, UART1_BASE + UART_DATA_OFFSET);
                    queueTask(td);
                    eventTable[UART1_XMIT_EVENT] = 0;
                }
            }
        }
        // UART 1 xmit
        else if (status & OR_XMIT_MASK)
        {
            // turn xmit interrupt off in UART
            setUARTCtrl(UART1_XMIT_EVENT, 0);

            if (ctsOn == 1)// && ctsOff)
            {
                // reset states
                ctsOn = 0;
                xmitRdy = 0;

                // we can safely send a bit
                // unblock if there is a guy waiting
                TaskDescriptor *td = eventTable[UART1_XMIT_EVENT];
                if (td != 0)
                {
                    taskSetRet(td, UART1_BASE + UART_DATA_OFFSET);
                    queueTask(td);
                    eventTable[UART1_XMIT_EVENT] = 0;
                }
            }
            // we are not CTS-clear.
            else
            {
                // mark transmit ready
                xmitRdy = 1;
            }
        }
        // UART 1 receive
        else if (status & OR_RECV_MASK)
        {
            char c = getUARTData(COM1);

            TaskDescriptor *td = eventTable[UART1_RECV_EVENT];
            if (td != 0)
            {
                // unblock notifier
                //td->ret = c;
                taskSetRet(td, c);
                queueTask(td);
                eventTable[UART1_RECV_EVENT] = 0;

                // turn interrupt off in UART
                setUARTCtrl(UART1_RECV_EVENT, 0);
            }
        }
    }
    // UART 2 receive
    else if (vic1Status & UART2_RECV_MASK)
    {
        // Get the character
        char c = getUARTData(COM2);

        TaskDescriptor *td = eventTable[UART2_RECV_EVENT];
        if (td != 0)
        {
            // unblock notifier
            //td->ret = c;
            taskSetRet(td, c);
            queueTask(td);
            eventTable[UART2_RECV_EVENT] = 0;

            // turn interrupt off in UART
            setUARTCtrl(UART2_RECV_EVENT, 0);
        }
    }
    // UART 2 transmit
    else if (vic1Status & UART2_XMIT_MASK)
    {
        // turn interrupt off in UART
        setUARTCtrl(UART2_XMIT_EVENT, 0);

        TaskDescriptor *td = eventTable[UART2_XMIT_EVENT];
        if (td != 0)
        {
            // unblock notifier
            taskSetRet(td, UART2_BASE + UART_DATA_OFFSET);
            queueTask(td);
            eventTable[UART2_XMIT_EVENT] = 0;
        }
    }
    // soft int
    else if (vic1Status & 1)
    {
        *(unsigned int *)(VIC1 + VIC_SOFT_INT_CLEAR) = 1;
    }
    else
    {
        // something is wrong here
        bwprintf(COM2, "Unable to handle unknown iterrupt.\n\r");
        for (;;);
    }
}
