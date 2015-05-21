#include <bwio.h>
#include <ts7200.h>
#include <stdbool.h>
#include <cpsr.h>
#include <task.h>
#include <scheduler.h>
#include <context_switch.h>

void firstTask() {
     // asm volatile( "swi 0" ); //  c-switch
     bwprintf( COM2, "UserTask\n" );
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
         task->cpsr = 0;
         task->next = NULL;
     }

     InitScheduler();
}

void HandleRequest(int request)
{

}

//void KernelExit()
//{
    //asm volatile();
//}

//void KernelEnter()
//{
    
//}

int main()
{
    //TaskDescriptor taskPool[MAX_NUM_TASKS];

    //InitKernel(taskPool);

    //TaskDescriptor *first = &taskPool[0];
    TaskDescriptor first;
    first.priority = 0;
    unsigned int *ptr = first.stack;
    first.sp = ptr+64;
    first.cpsr = 1610612944;
    int i;
    for (i=0;i < USER_STACK_SIZE ;i++)
    {
        first.stack[i] = (unsigned int)(firstTask);
    }
  bwputr(COM2, first.stack[1]);
    int request;
    KernelExit(&first, &request);

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
