#include <ts7200.h>
#include <stdbool.h>
#include <cpsr.h>
#include <task.h>
#include <syscall.h>
#include <scheduler.h>
#include <context_switch.h>

void firstTask() {
    bwprintf(COM2, "First task!\n\r");
    volatile int tid = MyTid();
    bwprintf(COM2, "Back! My tid: %d\n\r", tid);
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

void HandleRequest(TaskDescriptor *td, int request)
{
    // TODO: Enums for requests?
    switch (request)
    {
    case SyscallCreate: // User task called Create()
        // TODO: CreateTask() function
        break;
    case SyscallMyTid:
        bwprintf(COM2, "tid: %d", td->id);
        td->ret = td->id;
        break;
    case SyscallMyParentTid:
        break;
    case SyscallPass:
        break;
    case SyscallExit:
        break;
    default:
        break;
    }
}

int main()
{
    TaskDescriptor taskPool[MAX_NUM_TASKS];

    InitKernel(taskPool);

    //TaskDescriptor *first = &taskPool[0];

    TaskDescriptor first;
    
    first.id = 5;
    first.priority = 0;
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

    volatile int request;
    // bwputr(COM2, &request);
    KernelExit(&first, &request);
    
    HandleRequest(&first, request);

    KernelExit(&first, &request);

    bwprintf(COM2, "Back to kernel main! bye bye!");
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
