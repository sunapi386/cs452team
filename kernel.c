#include <bwio.h>
#include <ts7200.h>
#include <stdbool.h>
#include <cpsr.h>

struct TaskDescriptor {
    // char name[32];
    // int id;
    // int parent_id;
    // int priority;
    unsigned int sp; // stackpointer
    unsigned int cpsr; // save this with the TD
    unsigned int ret;
    // struct TaskDescriptor *next; // linked list style

    // enum state {
        // TASK_READY,
        // TASK_ACTIVE,
        // TASK_ZOMBIE,
    // };

};

void userTask() {
    // asm volatile( "swi 0" ); //  c-switch

    bwprintf( COM2, "UserTask\n" );
}



int main() {
    // bwsetfifo( COM2, OFF );
    bwprintf( COM2, "Xing in.. " );
    asm volatile (
        "stmfd sp!, {r3-r12, r14}\n\t"  // 1 Push kregs on kstack
        "ldmfd sp!, {r3-r12, r14}\n\t"  // 1 Pop kregs off kstack
    );
    bwprintf( COM2, "Came out" );
    return 0;
}
