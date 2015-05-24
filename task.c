#include <task.h>
#include <cpsr.h>

#define COM2 1

static int global_next_unique_task_id;
static unsigned int *global_current_stack_address;
static TaskDescriptor global_task_table[TASK_MAX_TASKS];

static inline unsigned int taskCalcStackHigh(int task_id) {
    unsigned int index = task_id;
    return TASK_STACK_HIGH - (TASK_STACK_SIZE * index + TASK_TRAP_SIZE);
}

static inline unsigned int taskCalcStackLow(int task_id) {
    unsigned int index = task_id;
    return TASK_STACK_HIGH - (TASK_STACK_SIZE * index + TASK_TRAP_SIZE);
}

// Make sure 0 <= {index,priority,unique} < TASK_{,PRIORITY,UNIQUE}_BITS
static inline int taskMakeId(int index, int priority, int unique) {
    return
        (index << TASK_INDEX_OFFSET) |
        (priority << TASK_PRIORITY_OFFSET) |
        (unique << TASK_UNIQUE_OFFSET);
}

// FIXME: Implement recycling here
static inline int taskFindFreeTaskTableIndex() {
    return global_next_unique_task_id;
}

void initTaskSystem() {
    global_next_unique_task_id = 1;
    global_current_stack_address = (unsigned int *) TASK_STACK_HIGH;

    for( int i = 1; i < TASK_MAX_TASKS; i++ ) {
        TaskDescriptor *task = global_task_table + i;
        task->id = 0;
        task->parent_id = 0;
        task->ret = 0;
        task->sp = NULL;
        task->next = NULL;
    }
}

TaskDescriptor *taskCreate(int priority, void (*code)(void), int parent_id) {
    if( priority < 0 || priority >= TASK_MAX_PRIORITY || code == NULL ) {
        bwprintf(COM2, "Invalid params for taskCreate()\n\r");
        return NULL;
    }
    if( global_next_unique_task_id >= TASK_MAX_TASKS ) {
        bwprintf(COM2, "Too many tasks\n\r");
        return NULL;
    }
    if( taskCalcStackHigh(global_next_unique_task_id) < TASK_STACK_LOW ||
        taskCalcStackLow(global_next_unique_task_id) > TASK_STACK_HIGH ) {
        bwprintf(COM2, "Stack out of bounds\n\r");
        return NULL;
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
    new_task->next = NULL;
    global_current_stack_address -= (TASK_TRAP_SIZE + TASK_STACK_SIZE);

    // FIXME: Shuo: init trap frame on stack for c-switch
    *(new_task->sp) = (unsigned int)code;                       // r1: pc
    *(new_task->sp + 1) = UserMode | DisableIRQ | DisableFIQ;   // r2: cpsr_user

    return new_task;
}

void taskExit() {
    // FIXME: Shuo: w
}
