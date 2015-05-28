#include <ts7200.h>
#include <stdbool.h>
#include <cpsr.h>
#include <task.h>
#include <syscall.h>
#include <context_switch.h>
#include <scheduler.h>
#include <bwio.h>
#include <user_task.h>
#include <utils.h>
#include <message_passing.h>
#include <message_benchmarks.h>


void initKernel() {
    // Initialize swi jump table to kernel entry point
    *(unsigned int *)(0x28) = (unsigned int)(&KernelEnter);

    initTaskSystem();
    initScheduler();
    initMessagePassing();

    // int create_ret = taskCreate(1, &userModeTask, 0);
    int create_ret = taskCreate(2, &rpsUserTask, 0);
    if( create_ret < 0 ) {
        bwprintf( COM2, "FATAL: fail creating first task.\n\r" );
        return;
    }
    bwprintf( COM2, "Creating first task %d\n\r", create_ret );
    queueTask(taskGetTDById(create_ret));
}

void handleRequest(TaskDescriptor *td, Syscall *request) {
    switch (request->type) {
        case SYS_CREATE: {
            int create_ret = taskCreate(request->arg1, (void*)request->arg2, taskGetIndex(td));
            if (create_ret >= 0) {
                td->ret = taskGetIndexById(create_ret);
                bwprintf(COM2, "Creating task %d\n\r", td->ret);
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

    for(task = schedule() ; ; task = schedule()) {
        Syscall **request = NULL;
        if (task == NULL) {
            bwprintf(COM2, "No tasks scheduled; exiting...\n\r");
            break;
        }

        KernelExit(task, request);
        handleRequest((TaskDescriptor *)task, *request);
    }

    return 0;
}
