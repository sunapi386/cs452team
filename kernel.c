#include <bwio.h>
#include <ts7200.h>
#include <stdbool.h>
#include <cpsr.h>
#include <task.h>
#include <scheduler.h>

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
        task->sp = NULL;
        task->cpsr = 0;
        task->next = NULL;
    }

    InitScheduler();
}

void HandleRequest(int request)
{

}

int main()
{
    TaskDescriptor taskPool[MAX_NUM_TASKS];
    InitKernel(taskPool);
    
    for (;;)
    {
        int request = 0;
        TaskDescriptor *task = Scheduler();
        
        if (task == NULL)
        {
            break;
        }

        //KernelExit(task, &request);
        HandleRequest(request);
    }

    return 0;
    // bwsetfifo( COM2, OFF );
    /*
    bwprintf( COM2, "Xing in.. " );
    asm volatile (
        "stmfd sp!, {r3-r12, r14}\n\t"  // 1 Push kregs on kstack
        "ldmfd sp!, {r3-r12, r14}\n\t"  // 1 Pop kregs off kstack
    );
    bwprintf( COM2, "Came out" );
    return 0;
*/
}
