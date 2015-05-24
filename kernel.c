#include <ts7200.h>
#include <stdbool.h>
#include <cpsr.h>
#include <task.h>
#include <syscall.h>
#include <scheduler.h>
#include <context_switch.h>

void firstTask() {
    bwprintf(COM2, "First task!\n\r");
    int tid = MyParentTid();
    bwprintf(COM2, "Back! My parent_tid: %d\n\r", tid);
    Exit();
    bwprintf(COM2, "Back again!");
    Exit();
}


void initKernel() {
    // Initialize swi jump table to kernel entry point
    *(unsigned int *)(0x28) = (unsigned int)(&KernelEnter);
    initTaskSystem();
    initScheduler();
}

void handleRequest(TaskDescriptor *td, Syscall *request)
{
    bwprintf(COM2, "Handling tid: %d\n\r", td->id);

    // TODO: Enums for requests?
    switch (request->type)
    {
    case SYS_CREATE:
        //td->ret = taskCreate(request->arg1, request->arg2, stack_size, parent_id);
        break;
    case SYS_MY_TID:
        bwprintf(COM2, "Setting return value for mytid: %d\n\r", td->id);
        td->ret = taskGetMyId(td);
        break;
    case SYS_MY_PARENT_TID:
        bwprintf(COM2, "Setting return value parent_tid: %d\n\r", td->parent_id);
        td->ret = taskGetMyParentId(td);
        break;
    case SYS_PASS:
        // Reschedule task
        enqueueTask(td);
        break;
    case SYS_EXIT:
        // Do not reschedule
        // taskExit();
        break;
    default:
        bwprintf(COM2, "Invalid syscall %u!", request->type);
        break;
   }
}

int main()
{
    initKernel();

    TaskDescriptor first;
    first.id = 1234;
    first.parent_id = 100;

    // r2 - pc (code address)
    *(first.sp - 11) = (unsigned int)(firstTask); // + 2195456

    // r3 - user cpsr
    *(first.sp - 10) = 16;

    // 12 registers will be poped from user stack
    first.sp -= 11;

    Syscall **request = 0;

    TaskDescriptor *td = &first;

    KernelExit(td, request);

    handleRequest(&first, *request);

    KernelExit(&first, request);

    bwprintf(COM2, "Kernel main! Try again!\n\r");

    KernelExit(&first, request);

    bwprintf(COM2, "Bye!\n\r");

    /*
    enqueueTask(first);

    for (;;)
    {
         int request = 0;
         TaskDescriptor *task = scheduler();

         if (task == NULL)
         {
             break;
         }

         KernelExit(task, &request);
         handleRequest(request);
    }*/
    return 0;
}
