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

static int senderTid = -1;
static int rcverTid = -1;

void snder()
{
    senderTid = MyTid();
    int data = 4;
    for (;;)
    {
        assert(rcverTid > 0);
        int ret = Send(rcverTid, &data, sizeof(int), 0, 0);
        assert(ret == 0);
        bwprintf(COM2, "+");
    }
}

void rcver()
{
    rcverTid = MyTid();
    int tid = 0;
    int buf = 0;
    for (;;)
    {
        int ret = Receive(&tid, &buf, sizeof(int));
        assert(ret == sizeof(int));
        assert(tid == senderTid);
        assert(buf == 4);
        bwprintf(COM2, ".");
        ret = Reply(tid, 0, 0);
        assert(ret == 0);
    }
}

void initTask()
{
    Create(16, rcver);
    Create(16, snder);
    Exit();
}

static void initKernel(TaskQueue *sendQueues) {
    cacheEnable();
    initTaskSystem();
    initScheduler();
    initMessagePassing(sendQueues);
    initInterrupts();
    initUART();
    initTimer();
    int create_ret = taskCreate(PRIORITY_INIT, initTask, 0);
    //int create_ret = taskCreate(PRIORITY_INIT, bootstrapTask, 0);
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
        // add the task back to the front of the ready queue
        addToFront(td);
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
            taskExit(td);
            return 0;
        case SYS_HALT:
            return -1;
        default:
            debug("Invalid syscall %u!", request->type);
            break;
        }
        // requeue the task if we haven't returned (from SYS_EXIT)
        queueTask(td);
    }
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
