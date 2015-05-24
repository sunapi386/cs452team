#include <task.h>
#include <cpsr.h>
#include <bwio.h>

static int global_next_unique_task_id;
static unsigned int *global_current_stack_address;
static TaskDescriptor global_task_table[TASK_MAX_TASKS];

void initTaskSystem(void (*firstTask)(void)) {
    global_next_unique_task_id = 1;
    global_current_stack_address = (unsigned int *) TASK_STACK_HIGH;
    // FIXME: should zero out the entire stack, high to low
    for( int i = 1; i < TASK_MAX_TASKS; i++ ) {
        TaskDescriptor *task = global_task_table + i;
        task->id = 0;
        task->parent_id = 0;
        task->ret = 0;
        task->sp = NULL;
        task->spsr = 0;
        task->next = NULL;
    }
    // setup first task, kernel_task
    int ret = taskCreate(1, firstTask, -1);
    if( ret < 0 ) {
        bwprintf( COM2, "FATAL: initTaskSystem fail creating first task %d.\n\r", ret );
    }
}

// FIXME: Implement recycling here
static inline int taskFindFreeTaskTableIndex() {
    return global_next_unique_task_id;
}

TaskDescriptor *taskCreate(int priority, void (*code)(void), int parent_id) {
    if( priority < 0 || priority >= TASK_MAX_PRIORITY || code == NULL ) {
        return -1; // invalid params
    }
    if( global_next_unique_task_id >= TASK_MAX_TASKS ) {
        return -2; // too many tasks
    }
    if( taskCalcStackHigh(global_next_unique_task_id) < TASK_STACK_LOW ||
        taskCalcStackLow(global_next_unique_task_id) > TASK_STACK_HIGH ) {
        return -3; // stack out of bounds
    }

    // FIXME: No recycling tasks ids
    // Once all the TASK_MAX_TASKS been given out, will fail to create new tasks
    int unique_id = global_next_unique_task_id++;
    int task_table_index = taskFindFreeTaskTableIndex();
    TaskDescriptor *new_task = &global_task_table[task_table_index];
    new_task->id = taskMakeId( task_table_index, priority, unique_id );
    new_task->parent_id = parent_id;
    new_task->ret = 0;
    new_task->sp = global_current_stack_address - TASK_TRAP_SIZE;
    new_task->spsr = UserMode | DisableIRQ | DisableFIQ;
    new_task->next = NULL;
    global_current_stack_address -= (TASK_TRAP_SIZE + TASK_STACK_SIZE);

    // FIXME: Shuo: init trap frame on stack for c-switch
    *(new_task->sp) = 0; //
    *(new_task->sp + 1) = 0; //
    *(new_task->sp + 2) = 0; //
    *(new_task->sp + 3) = 0; //
    *(new_task->sp + 4) = 0; //
    *(new_task->sp + 5) = 0; //
    *(new_task->sp + 6) = 0; //
    *(new_task->sp + 7) = 0; //
    *(new_task->sp + 8) = 0; //
    *(new_task->sp + 9) = 0; //
    *(new_task->sp + 10) = 0; //
    *(new_task->sp + 11) = 0; //
    *(new_task->sp + 12) = 0; //
    *(new_task->sp + 13) = 0; //
    *(new_task->sp + 14) = 0; //

    *(new_task->sp) = new_task->sp + 15; // FIXME

    return new_task;
}


void taskExit() {
    // FIXME: Shuo: w
}
