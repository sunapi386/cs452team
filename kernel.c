#include <bwio.h>
#include <ts7200.h>
#include <stdbool.h>
#include <cpsr.h>
#include <task.h>
#include <scheduler.h>
#include <context_switch.h>

void firstTask() {
    // asm volatile( "swi 0" ); //  c-switch
    bwprintf(COM2, "UserTask\n\r");
    //asm volatile ("swi 12");
}

void InitKernel(TaskDescriptor pool[])
{
     // Initialize swi jump table
     //*(unsigned int)(0x28) = &kernel_entry;

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

void HandleRequest(int request)
{

}

int main()
{
    //TaskDescriptor taskPool[MAX_NUM_TASKS];

    //InitKernel(taskPool);

    //TaskDescriptor *first = &taskPool[0];
    // *(unsigned int *)(0x28) = 2195456 + (unsigned int)(&KernelEnter);
    
    TaskDescriptor first;
    
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
    *(first.sp - 11) = 2195456 + (unsigned int)(firstTask);
    
    // r3 - user cpsr
    *(first.sp - 10) = 16;

    first.sp -= 11;

    //bwputr(COM2, first.sp);
    //bwprintf(COM2, "sp-1 val: %d\n",*(first.sp-1));

    int request;
    KernelExit(&first, &request);

    //bwprintf(COM2, "Request: %d", request);
    
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
