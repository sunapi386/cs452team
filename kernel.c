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


void InitKernel(TaskDescriptor pool[])
{
    // Initialize swi jump table to kernel entry point
    *(unsigned int *)(0x28) = (unsigned int)(&KernelEnter);

    // Initialize tasks
    int i;
    for (i = 0; i < MAX_NUM_TASKS; i++)
    {
        TaskDescriptor *task = &pool[i];
        task->id = i;
        task->parent_id = 0;
        task->priority = 0;
        task->ret = 0;
        task->sp = &(task->stack[255]);
        task->next = NULL;
    }

    InitScheduler();
}

void HandleRequest(TaskDescriptor *td, Syscall *request)
{
    bwprintf(COM2, "Handling tid: %d\n\r", td->id);
    
    // TODO: Enums for requests?
    switch (request->type)
    {
    case SyscallCreate: // User task called Create()
        // TODO: CreateTask() function
        break;
    case SyscallMyTid:
        bwprintf(COM2, "Setting return value for mytid: %d\n\r", td->id);
        td->ret = td->id;
        request->ret = td->id;
        break;
    case SyscallMyParentTid:
        bwprintf(COM2, "Setting return value parent_tid: %d\n\r", td->parent_id);
        td->ret = td->parent_id;
        request->ret = td->parent_id;
        break;
    case SyscallPass:
        break;
    case SyscallExit:
        break;
    default:
        break;
    }
}

void printStack(unsigned int *ptr)
{
     int j;
    for (j =220; j < 256; j++)
    {
        bwprintf(COM2, "%d : %d\n\r", j, *(ptr + j));
    }
}

void printTaskInfo(TaskDescriptor *td)
{
    bwprintf(COM2, "id: %d\n\r", td->id);
bwprintf(COM2, "parent_id: %d\n\r", td->parent_id);
bwprintf(COM2, "priority: %d\n\r", td->priority);
bwprintf(COM2, "ret: %d\n\r", td->ret);
bwprintf(COM2, "sp: %x\n\r", td->sp);
}

int main()
{
    TaskDescriptor taskPool[MAX_NUM_TASKS];

    InitKernel(taskPool);

    //TaskDescriptor *first = &taskPool[0];

    TaskDescriptor first;
    
    first.id = 1234;
    first.parent_id = 100;
    first.priority = 0;
    first.ret = 17;
    unsigned int *ptr = first.stack;
    first.sp = ptr + USER_STACK_SIZE - 1;

    int i;
    for (i=0;i < USER_STACK_SIZE ;i++)
    {
        first.stack[i] = i;
        //first.stack[i] = 2195456 + (unsigned int)(firstTask);
    }

    // r2 - pc (code address)
    *(first.sp - 11) = (unsigned int)(firstTask); // + 2195456

    // r3 - user cpsr
    *(first.sp - 10) = 16;
    
    // 12 registers will be poped from user stack
    first.sp -= 11;

    Syscall **request = 0;
    
    bwputr(COM2, &(first.id));

    //printTaskInfo(&first);
    TaskDescriptor *td = &first;
    KernelExit(td, request);
    //printTaskInfo(&first);

    HandleRequest(&first, *request);

    KernelExit(&first, request);

    bwprintf(COM2, "Kernel main! Try again!\n\r");

    KernelExit(&first, request);

    bwprintf(COM2, "Bye!\n\r");

    /*
    EnqueueTask(first);

    for (;;)
    {
         int request = 0;
         TaskDescriptor *task = Scheduler();

         if (task == NULL)
         {
             break;
         }
        
         KernelExit(task, &request);
         HandleRequest(request);
    }*/
    return 0;
}
