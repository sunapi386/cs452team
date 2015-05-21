#include <bwio.h>
#include <ts7200.h>
#include <stdbool.h>
#include <cpsr.h>
#include <task.h>
#include <scheduler.h>

// void firstTask() {
//     // asm volatile( "swi 0" ); //  c-switch

//     bwprintf( COM2, "UserTask\n" );
// }

// void InitKernel(TaskDescriptor pool[])
// {
//     // Initialize swi jump table
//     //*(unsigned int)(0x28) = &kernel_entry;

//     // Initialize tasks
//     int i;
//     for (i = 0; i < MAX_NUM_TASKS; i++)
//     {
//         TaskDescriptor *task = &pool[i];
//         task->id = i;
//         task->parent_id = 0;
//         task->priority = 0;
//         task->ret = 0;
//         task->sp = NULL;
//         task->cpsr = 0;
//         task->next = NULL;
//     }

//     InitScheduler();
// }

// void HandleRequest(int request)
// {

// }

int main()
{
     asm volatile(
        "KernelExit:\n\t"
        "mov ip, sp\n\t"
        "stmfd sp!, {fp, ip, lr, pc}\n\t"  // save kregs on kstack
        "sub fp, ip, #4\n\t"
        "sub sp, sp, #12\n\t"
        "str r1, [fp, #-24]\n\t"
        "str r0, [fp, #-20]\n\t"
        "str r1, [fp, #-24]\n\t"
        "ldr r3, [fp, #-20]\n\t"
        "ldr r3, [r3, #20]\n\t"
        "str r3, [fp, #-16]\n\t"
        "ldr r3, [fp, #-16]\n\t"
        "mov r0, #1\n\t"
        "mov r1, r3\n\t"
        "bl bwputr(PLT)\n\t"
        "sub sp, fp, #12\n\t"
        "ldmfd sp, {fp, sp, pc}\n\t"
        "KernelEnter:\n\t"
        "mov ip, sp\n\t"
        "stmfd sp!, {fp, ip, lr, pc}\n\t"
        "sub fp, ip, #4\n\t"
        "ldmfd sp, {fp, sp, pc}\n\t"
    );



    // TaskDescriptor taskPool[MAX_NUM_TASKS];
    // InitKernel(taskPool);

    // for (;;)
    // {
    //     int request = 0;
    //     TaskDescriptor *task = Scheduler();

    //     if (task == NULL)
    //     {
    //         break;
    //     }

    //     //KernelExit(task, &request);
    //     HandleRequest(request);
    // }

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
