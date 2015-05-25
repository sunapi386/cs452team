#include <ts7200.h>
#include <stdbool.h>
#include <cpsr.h>
#include <task.h>
#include <syscall.h>
#include <context_switch.h>
#include <scheduler.h>
#include <bwio.h>
#include <user_task.h>

void initKernel() {
    // Initialize swi jump table to kernel entry point
    *(unsigned int *)(0x28) = (unsigned int)(&KernelEnter);
    initTaskSystem();

    initScheduler();

    // setup first task, kernel_task
    TaskDescriptor *initialTask = taskCreate(1, &userModeTask, -1);
    if( ! initialTask ) {
        bwprintf( COM2, "FATAL: fail creating first task.\n\r" );
        return;
    }
    queueTask(initialTask);
}

void handleRequest(TaskDescriptor *td, Syscall *request) {
    switch (request->type) {
    case SYS_CREATE: {
        TaskDescriptor *task = taskCreate(request->arg1, (void*)request->arg2, td->parent_id);
        if (task) {
            queueTask(task);
            td->ret = taskGetUnique(task);
        }
        else {
            td->ret = -1;
        }
        break;
    }
    case SYS_MY_TID:
        td->ret = taskGetUnique(td);
        break;
    case SYS_MY_PARENT_TID:
        td->ret = taskGetMyParentUnique(td);
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

int main()
{
    initKernel();

    volatile TaskDescriptor *task = NULL;
    for(task = schedule(); ; task = schedule()) {
         Syscall **request = NULL;
         if (task == NULL)  {
             bwprintf(COM2, "No tasks scheduled; exiting...\n\r");
             break;
         }
         KernelExit(task, request);
         handleRequest(task, *request);
    }

    return 0;
}
