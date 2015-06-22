#define KERNEL_MAIN
#include <kernel/scheduler.h>
#include <kernel/message_passing.h>
#include <user/syscall.h>
#undef KERNEL_MAIN
#include <kernel/interrupts.h>
#include <kernel/context_switch.h>
#include <kernel/uart.h>
#include <kernel/timer.h>
#include <user/user_tasks.h>
#include <user/io.h>
#include <debug.h>
#include <events.h>
#include <user/clockserver.h>
#include <user/nameserver.h>
#include <priority.h>
#include <user/clock_drawer.h>
#include <user/parser.h>
#include <user/sensor.h>


static Syscall *request = NULL;

void enableCache()
{
    asm volatile(
        "mrc p15, 0, r0, c1, c0, 0\n\t"
        "ldr r1, =0x00001004\n\t"
        "orr r0, r0, r1\n\t"
        "mcr p15, 0, r0, c1, c0, 0\n\t"
    );
}

void disableCache()
{
    asm volatile(
        "mrc p15, 0, r0, c1, c0, 0\n\t"
        "ldr r1, =0xffffeffb\n\t"
        "and r0, r0, r1\n\t"
        "mcr p15, 0, r0, c1, c0, 0\n\t"
    );
}

void idleProfiler() {
    for (;;) {
        //drawIdle(getIdlingRatio());
        //bwprintf(COM2, "_");
        //Putc(COM2, 'i');
        Pass();
    }
}

#include <ts7200.h>

static int clientTid = -1;

void client()
{
    for (;;)
    {
        char *dataAddr = (char *)AwaitEvent(UART2_XMIT_EVENT);
        if (dataAddr != (char *)(UART2_BASE + UART_DATA_OFFSET))
        {
            bwprintf(COM2, "gg %x", dataAddr);
            bwprintf(COM2, "my tid: %d, original tid: %d", MyTid(), clientTid);
            break;
        }
        *dataAddr = ' ';
        //Putc(COM2, '*');
    }
    Exit();
}

void clientNotifier()
{
    //char *dataAddr = (char *)(UART2_BASE + UART_DATA_OFFSET);
    /*
        char buf[2048];
        CBuffer rb;
        CBufferInit(&rb, buf, 2048);
    */
    // unsigned int i;
    //clientTid = clientTid;
    for (;;)
    {
        //AwaitEvent(UART2_XMIT_EVENT);
        //*dataAddr = '*';
        Pass();
    }
    Exit();
}

void bootstrap()
{
    // Create name server
    //Create (PRIORITY_NAMESERVER, nameserverTask);

    // Create clock server
    Create(PRIORITY_CLOCK_SERVER, clockServerTask);

    // Create IO Servers
    //Create(PRIORITY_TRAIN_OUT_SERVER, trainOutServer);
    //Create(PRIORITY_TRAIN_IN_SERVER, trainInServer);
    //Create(PRIORITY_MONITOR_OUT_SERVER, monitorOutServer);
    //Create(PRIORITY_MONITOR_IN_SERVER, monitorInServer);

    // Create user task
    //Create(PRIORITY_CLOCK_DRAWER, clockDrawer);
    //Create(PRIORITY_PARSER, parserTask);
    // Create(PRIORITY_SENSOR_TASK, sensorTask);

    Create(PRIORITY_USERTASK, client);
    Create(PRIORITY_USERTASK, clientNotifier);
    //Create(PRIORITY_USERTASK, client2);

    // Create idle task
    Create(PRIORITY_IDLE, idleProfiler);

    // quit
    Exit();
}

static void initKernel() {
    enableCache();
    initTaskSystem();
    initScheduler();
    initMessagePassing();
    request = initSyscall();
    initInterrupts();
    initUART();
    initTimer();

    //int create_ret = taskCreate(1, userTaskMessage, 0);
    // int create_ret = taskCreate(1, userTaskHwiTester, 0);
    // int create_ret = taskCreate(1, runBenchmarkTask, 0);
    // int create_ret = taskCreate(1, interruptRaiser, 0);
    // int create_ret = taskCreate(1, userTaskK3, 0);
    // int create_ret = taskCreate(1, userTaskIdle, 31);
    int create_ret = taskCreate(PRIORITY_INIT, bootstrap, 0);

    assert(create_ret >= 0);
    queueTask(taskGetTDById(create_ret));
}

static void resetKernel() {
    resetTimer();
    // resetUART();
    resetInterrupts();
    disableCache();
}

static inline int handleRequest(TaskDescriptor *td) {
    if (isHwi())
    {
        handleInterrupt();
        clearHwi();
        goto done;
    }

    switch (request->type) {
        case SYS_AWAIT_EVENT:
            if (awaitInterrupt(td, request->arg1) == -1)
            {
                // we want rescheduling
                td->ret = -1;
                break;
            }
            // we don't want rescheduling now that it's event blocked
            return 0;
        case SYS_SEND:
            handleSend(td, request);
            return 0;
        case SYS_RECEIVE:
            handleReceive(td, request);
            return 0;
        case SYS_REPLY:
            handleReply(td, request);
            return 0;
        case SYS_CREATE: {
            int create_ret = taskCreate(request->arg1,
                (void*)(request->arg2),
                taskGetIndex(td));
            if (create_ret >= 0) {
                td->ret = taskGetIndexById(create_ret);
                queueTask(taskGetTDById(create_ret));
            } else {
                td->ret = create_ret;
            }
            break;
        }
        case SYS_MY_TID:
            td->ret = taskGetIndex(td);
            break;
        case SYS_MY_PARENT_TID:
            td->ret = taskGetMyParentIndex(td);
            break;
        case SYS_PASS:
            break;
        case SYS_EXIT:
            return 0;
        case SYS_HALT:
            return -1;
        default:
            debug("Invalid syscall %u!", request->type);
            break;
    }
done:
    // requeue the task if we haven't returned (from SYS_EXIT)
    queueTask(td);
    return 0;
}

int main() {
    initKernel();
    TaskDescriptor *task = NULL;
    for(;;) {
        task = schedule();
        if (task == NULL) {
            break;
        }
        kernelExit(task);
        if(handleRequest(task)) {
            bwprintf(COM2, "Halt\n\r");
            break;
        }
    }
    resetKernel();
    debug("No tasks scheduled; exiting...");
    bwputc(COM1, 0x61);
    return 0;
}
