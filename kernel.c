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
    bwputr( COM2, "UserTask\n" );
    asm volatile( "swi 0" ); //  c-switch
}

static void initTask(struct TaskDescriptor *task) {
    // task owns stack from 0x300000 to 0x400000, stack starting at 0x400000
    task->ret = 0;
    task->cpsr = UserMode;
    task->sp = 0x400000;

}



int main( int argc, char* argv[] ) {
    bwsetfifo( COM2, OFF );
    bwputr( COM2, "UserTask\n" );

    return 0;
}
