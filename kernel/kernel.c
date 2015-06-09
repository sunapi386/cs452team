#define KERNEL_MAIN
#include <kernel/scheduler.h>
#include <kernel/message_passing.h>
#include <user/syscall.h>
#undef KERNEL_MAIN
#include <kernel/interrupts.h>
#include <kernel/context_switch.h>
#include <kernel/timer.h>
#include <user/user_tasks.h>
#include <debug.h>
#include <stdbool.h>

static Syscall *request = NULL;

void enableCache()
{
    asm volatile(
        "mrc p15, 0, r0, c1, c0, 0\n\t"
        "ldr r1, =0xc0001004\n\t"
        "orr r0, r0, r1\n\t"
        "mcr p15, 0, r0, c1, c0, 0\n\t"
    );
}

void disableCache()
{
    asm volatile(
        "mrc p15, 0, r0, c1, c0, 0\n\t"
        "ldr r1, =0x1fffeffb\n\t"
        "and r0, r0, r1\n\t"
        "mcr p15, 0, r0, c1, c0, 0\n\t"
    );
}

void taskTest(volatile int arg)
{
    debug("my arg is %d! ;)", arg);
    Exit();
}

void bootstrap()
{
    int ret = Spawn(1, &taskTest, -500);
    debug("bootstrap retval %d", ret);
    Exit();
}

static void initKernel() {
    bwsetfifo(COM2, false);
    enableCache();
    initTaskSystem();
    initScheduler();
    initMessagePassing();
    request = initSyscall();
    initInterrupts();
    initTimer();

    //int create_ret = taskCreate(1, &userTaskMessage, 0);
    // int create_ret = taskCreate(1, &userTaskHwiTester, 0);
    // int create_ret = taskCreate(1, &runBenchmarkTask, 0);
    // int create_ret = taskCreate(1, &interruptRaiser, 0);
    // int create_ret = taskCreate(1, &userTaskK3, 0);
    // int create_ret = taskCreate(1, &userTaskIdle, 31);
    //int create_ret = taskCreate(1, &userTaskK3, 0);
    int create_ret = taskCreate(31, &bootstrap, 0);

    assert(create_ret >= 0);
    queueTask(taskGetTDById(create_ret));
}

static void resetKernel() {
    stopTimer();
    resetInterrupts();
    disableCache();
}

static inline void handleRequest(TaskDescriptor *td) {
    switch (request->type) {
        case INT_IRQ:
            handleInterrupt();
            break;
        case SYS_AWAIT_EVENT:
            td->ret = awaitInterrupt(td, request->arg1);
            // we don't want to reschedule if the task is event blocked
            if (td->ret == 0) return;
        case SYS_SEND:
            handleSend(td, request);
            return;
        case SYS_RECEIVE:
            handleReceive(td, request);
            return;
        case SYS_REPLY:
            handleReply(td, request);
            return;
        case SYS_CREATE: {
            int create_ret = taskCreate(request->arg1, (void*)(request->arg2), taskGetIndex(td));
            if (create_ret >= 0) {
                td->ret = taskGetIndexById(create_ret);
                queueTask(taskGetTDById(create_ret));
            } else {
                td->ret = create_ret;
            }
            break;
        }
        case SYS_SPAWN: {
            TaskDescriptor *task = taskSpawn(request->arg1,
                                             (void *)(request->arg2),
                                             (void *)(request->arg3),
                                             taskGetIndex(td));
            queueTask(task);
            td->ret = task == NULL ? -1 : 0;
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
            return;
        default:
            debug("Invalid syscall %u!", request->type);
            break;
    }
    // requeue the task if we haven't returned (from SYS_EXIT)
    queueTask(td);
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
        handleRequest(task);
        request->type = INT_IRQ;
    }
    resetKernel();
    debug("No tasks scheduled; exiting...");
    return 0;
}
