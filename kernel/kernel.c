#define KERNEL_MAIN
#include <kernel/scheduler.h>
#include <kernel/message_passing.h>
#undef KERNEL_MAIN
#include <user/syscall.h>
#include <kernel/interrupts.h>
#include <kernel/context_switch.h>
#include <kernel/uart.h>
#include <kernel/timer.h>
#include <debug.h>
#include <priority.h>
#include <kernel/bootstrap.h>
#include <kernel/cache.h>
#include <user/clockserver.h>

static void initKernel(TaskQueue *sendQueues) {
    cacheEnable();
    initTaskSystem();
    initScheduler();
    initMessagePassing(sendQueues);
    initInterrupts();
    initUART();
    initTimer();

    int create_ret = taskCreate(PRIORITY_INIT, bootstrapTask, 0);
    queueTask(taskGetTDById(create_ret));
}

static void resetKernel() {
    bwputc(COM1, 0x61);
    resetTimer();
    resetInterrupts();
    resetUART();
    cacheDisable();
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
