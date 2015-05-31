#define KERNEL_MAIN
#include <scheduler.h>
#include <message_passing.h>
#include <syscall.h>
#undef KERNEL_MAIN
#include <ts7200.h>
#include <stdbool.h>
#include <cpsr.h>
#include <context_switch.h>
#include <bwio.h>
#include <user_task.h>
#include <utils.h>
#include <message_benchmarks.h>

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
    // Initialize swi jump table to kernel entry point
    *(unsigned int *)(0x28) = (unsigned int)(&KernelEnter);

    initTaskSystem();
    initScheduler();
    initMessagePassing();
    request = initSyscall();

    //int create_ret = taskCreate(1, &userModeTask, 0);
    //int create_ret = taskCreate(2, &rpsUserTask, 0);
    int create_ret = taskCreate(1, &runBenchmark, 0);
    if( create_ret < 0 ) {
        bwprintf( COM2, "FATAL: fail creating first task.\n\r" );
        return;
    }
    queueTask(taskGetTDById(create_ret));
}

static inline void handleRequest(TaskDescriptor *td) {
    switch (request->type) {
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
        case SYS_SEND:
            handleSend(td, request);
            return;
        case SYS_RECEIVE:
            handleReceive(td, request);
            return;
        case SYS_REPLY:
            handleReply(td, request);
            return;
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
        task = schedule();
        if (task == NULL) {
            break;
        }
        KernelExit(task);
        handleRequest(task);
    }
#if ENABLE_CACHE
    disableCache();
#endif
    bwprintf(COM2, "No tasks scheduled; exiting...\n\r");
    return 0;
}
