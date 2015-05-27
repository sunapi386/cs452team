#include <task.h>
#include <cpsr.h>
#include <bwio.h>

static int global_next_unique_task_id;
static unsigned int *global_current_stack_address;
static TaskDescriptor global_task_table[TASK_MAX_TASKS];

// FIXME: Implement recycling here
static inline int taskFindFreeTaskTableIndex() {
    return global_next_unique_task_id++;
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
        task->status = none;
        task->send_id = NULL;
        task->send_buf = NULL;
        task->recv_buf = NULL;
        task->send_len = 0;
        task->recv_len = 0;
        task->next = NULL;
    }
}

// IMPROVE: Implement recycling here
static inline int taskFindFreeTaskTableIndex() {
    return global_next_unique_task_id;
}

int taskCreate(int priority, void (*code)(void), int parent_id) {
    if( priority < 0 || priority >= TASK_MAX_PRIORITY || code == NULL ) {
        bwprintf( COM2, "FATAL: create task bad priority %d.\n\r", priority );
        return -1; // invalid params
    }
    if( global_next_unique_task_id >= TASK_MAX_TASKS ) {
        bwprintf( COM2, "FATAL: too many tasks %d.\n\r", global_next_unique_task_id );
        return -2; // too many tasks
    }
    unsigned int boundary = (unsigned int)(global_current_stack_address - TASK_STACK_SIZE - TASK_TRAP_SIZE);

    if( boundary < TASK_STACK_LOW ){
        bwprintf( COM2, "FATAL: at low stack boundary 0x%x.\n\r", boundary );
        return -3; // stack out of bounds
    }
    // IMPROVE: No recycling tasks ids
    // Once all the TASK_MAX_TASKS been given out, will fail to create new tasks
    int unique_id = global_next_unique_task_id;
    int task_table_index = taskFindFreeTaskTableIndex();
    TaskDescriptor *new_task = &global_task_table[task_table_index];
    new_task->id = makeId( task_table_index, priority, unique_id );
    new_task->parent_id = parent_id;
    new_task->ret = 0;
    new_task->sp = global_current_stack_address - TASK_TRAP_SIZE;
    new_task->next = NULL;
    global_current_stack_address -= (TASK_TRAP_SIZE + TASK_STACK_SIZE);

    // init trap frame on stack for c-switch
    *(new_task->sp) = (unsigned int)code;                       // r1: pc
    *(new_task->sp + 1) = UserMode | DisableIRQ | DisableFIQ;   // r2: cpsr_user

    return new_task->id;
}

inline int taskGetMyId(TaskDescriptor *task) {
    return task->id;
}

inline int taskGetPriority(TaskDescriptor *task) {
    return (task->id & TASK_PRIORITY_MASK) >> TASK_PRIORITY_OFFSET;
}

inline int taskGetMyParentId(TaskDescriptor *task) {
    return task->parent_id;
}

inline void taskSetReturnValue(TaskDescriptor *task, int ret) {
    task->ret = ret;
}

TaskDescriptor *taskGetTDByIndex(int index) {
    if (index < 0 || index >= TASK_MAX_TASKS) {
        return NULL;
    }
    return global_task_table + index;
}

TaskDescriptor *taskGetTDById(int task_id) {
    int index = task_id & TASK_INDEX_MASK;
    if( index < 0 || index >= TASK_MAX_TASKS ) {
        return NULL;
    }
    return global_task_table + index;
}

int taskGetIndex(TaskDescriptor *task) {
    return (TASK_INDEX_MASK & task->id) >> TASK_INDEX_OFFSET;
}

int taskGetMyParentIndex(TaskDescriptor *task) {
    return (TASK_INDEX_MASK & task->parent_id) >> TASK_INDEX_OFFSET;
}

int taskGetUnique(TaskDescriptor *task) {
    return (TASK_UNIQUE_MASK & task->id) >> TASK_UNIQUE_OFFSET;
}

int taskGetMyParentUnique(TaskDescriptor *task) {
    return (TASK_UNIQUE_MASK & task->parent_id) >> TASK_UNIQUE_OFFSET;
}
