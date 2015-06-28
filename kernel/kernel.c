#define KERNEL_MAIN
#include <kernel/scheduler.h>
#include <kernel/message_passing.h>
#undef KERNEL_MAIN
#include <user/syscall.h>
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
#include <user/train.h>
#include <kernel/pl190.h>

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

void idleProfiler()
{
    for (;;)
    {
        drawIdle(taskIdleRatio());
    }
    Exit();
}

void bootstrap()
{
    // Create name server
    Create(PRIORITY_NAMESERVER, nameserverTask);

    // Create clock server
    //Create(PRIORITY_CLOCK_SERVER, clockServerTask);

    // Create IO Servers
    Create(PRIORITY_TRAIN_OUT_SERVER, trainOutServer);
    Create(PRIORITY_TRAIN_IN_SERVER, trainInServer);
    Create(PRIORITY_MONITOR_OUT_SERVER, monitorOutServer);
    Create(PRIORITY_MONITOR_IN_SERVER, monitorInServer);

    // Create user task
    Create(PRIORITY_CLOCK_DRAWER, clockDrawer);
    Create(PRIORITY_PARSER, parserTask);
    //Create(PRIORITY_SENSOR_TASK, sensorTask);

    // Create idle task
    Create(PRIORITY_IDLE, idleProfiler);

    // Quit
    Exit();
}

static void initKernel(TaskQueue *sendQueues) {
    enableCache();
    initTaskSystem();
    initScheduler();
    initMessagePassing(sendQueues);
    initInterrupts();
    initUART();
    initTimer();
    initTrain();

    //int create_ret = taskCreate(PRIORITY_INIT, userTaskMessage, 0);
    //int create_ret = taskCreate(PRIORITY_INIT, userTaskHwiTester, 0);
    // int create_ret = taskCreate(1, runBenchmarkTask, 0);
    // int create_ret = taskCreate(1, interruptRaiser, 0);
    //int create_ret = taskCreate(0, userTaskK3, 0);
    // int create_ret = taskCreate(1, userTaskIdle, 31);
    int create_ret = taskCreate(PRIORITY_INIT, bootstrap, 0);

    //int create_ret = taskCreate(PRIORITY_INIT, msg_stress, 0);

    queueTask(taskGetTDById(create_ret));
}

static void resetKernel() {
    bwputc(COM1, 0x61);
    resetTimer();
    resetInterrupts();
    resetUART();
    disableCache();
}

int handleRequest(TaskDescriptor *td, Syscall *request, TaskQueue *sendQueues) {

    if (request == NULL)
    {
        handleInterrupt();
    }
    else
    {
        switch (request->type)
        {
        case SYS_AWAIT_EVENT:
            if (awaitInterrupt(td, request->arg1) == -1)
            {
                taskSetRet(td, -1);
                break;
            }
            // we don't want rescheduling now that it's event blocked
            return 0;
        case SYS_SEND:
            handleSend(sendQueues, td, request);
            return 0;
        case SYS_RECEIVE:
            handleReceive(sendQueues, td, request);
            return 0;
        case SYS_REPLY:
            handleReply(td, request);
            return 0;
        case SYS_CREATE:
        {
            int create_ret = taskCreate(request->arg1,
                (void*)(request->arg2),
                taskGetIndex(td));
            if (create_ret >= 0)
            {
                taskSetRet(td, taskGetIndexById(create_ret));
                queueTask(taskGetTDById(create_ret));
            }
            else
            {
                taskSetRet(td, create_ret);
            }
            break;
        }
        case SYS_MY_TID:
            taskSetRet(td, taskGetIndex(td));
            break;
        case SYS_MY_PARENT_TID:
            taskSetRet(td, taskGetMyParentIndex(td));
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
    }

    // requeue the task if we haven't returned (from SYS_EXIT)
    queueTask(td);
    return 0;
}

int main()
{
    TaskQueue sendQueues[TASK_MAX_TASKS];

    initKernel(sendQueues);
    TaskDescriptor *task = NULL;
    Syscall *request = NULL;

    unsigned int task_begin_time;
    for(;;)
    {
        task = schedule();
        if (task == NULL) {
            debug("No tasks scheduled; exiting...");
            break;
        }

        task_begin_time = clockServerGetTick();
        request = kernelExit(task);
        task->cpu_time_used += (clockServerGetTick() - task_begin_time);

        if(handleRequest(task, request, sendQueues)) {
            debug("Halt");
            break;
        }
    }
    resetKernel();
    return 0;
}
