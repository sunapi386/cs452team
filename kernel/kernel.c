#define KERNEL_MAIN
#include <kernel/scheduler.h>
#include <kernel/message_passing.h>
#include <user/syscall.h>
#undef KERNEL_MAIN
#include <kernel/interrupts.h>
#include <kernel/context_switch.h>
#include <bwio.h>
#include <user/all_user_tasks.h>
#include <kernel/timer.h>

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

static void initKernel() {
#if ENABLE_CACHE
    enableCache();
#endif
    initTaskSystem();
    initScheduler();
    initMessagePassing();
    request = initSyscall();
    initInterrupts();

    // int create_ret = taskCreate(1, &userTaskMessage, 0);
    // int create_ret = taskCreate(1, &hwiTester, 0);
    // int create_ret = taskCreate(1, &runBenchmark, 0);
    // int create_ret = taskCreate(1, &interruptRaiser, 0);
    int create_ret = taskCreate(1, &userTaskK3, 0);

    if( create_ret < 0 ) {
        bwprintf( COM2, "FATAL: fail creating first task.\n\r" );
        return;
    }
    queueTask(taskGetTDById(create_ret));
}

static inline void handleRequest(TaskDescriptor *td) {
    switch (request->type) {
        case IRQ:
            handleInterrupt(td); // see AwaitEvent and event queue
            break;
        case SYS_AWAIT_EVENT:
            // puts an event in the queue into the queue that correspondes with the event
            unsigned int eventType = request->arg1;
            int ret = awaitInterrupt(td, eventType);
            if(ret == eventType) {  } // FIXME: conditionally requeue active task
            else { } //
            break;
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
            bwprintf(COM2, "Invalid syscall %u!", request->type);
            break;
    }

    // requeue the task if we haven't returned (from SYS_EXIT)
    queueTask(td);
}

int main() {
    initKernel();
    TaskDescriptor *task = NULL;
    for(;;) {
        // bwprintf(COM2, "[main] trying to schedule task %x\n\r", task);
        task = schedule();
        if (task == NULL) {
            break;
        }
        KernelExit(task);
        handleRequest(task);
        request->type = IRQ;
    }
#if ENABLE_CACHE
    disableCache();
#endif
    bwprintf(COM2, "No tasks scheduled; exiting...\n\r");
    return 0;
}
