#include <kernel/task.h>
#include <cpsr.h>
#include <bwio.h>
#include <utils.h>
#include <priority.h> // PRIORITY_IDLE

static int global_next_unique_task_id;
static unsigned int *global_current_stack_address;
static TaskDescriptor global_task_table[TASK_MAX_TASKS];

inline int isValidTaskIndex(int index) {
    return (index > 0) && (index < TASK_MAX_TASKS);
}

// Make sure 0 <= {index,priority,unique} < TASK_{,PRIORITY,UNIQUE}_BITS
inline int makeId(int index, int priority, int unique) {
    return  (index << TASK_INDEX_OFFSET) |
            (priority << TASK_PRIORITY_OFFSET) |
            (unique << TASK_UNIQUE_OFFSET);
}

inline int taskGetIndex(TaskDescriptor *task) {
    return (TASK_INDEX_MASK & task->id) >> TASK_INDEX_OFFSET;
}

inline int taskGetPriority(TaskDescriptor *task) {
    return (task->id & TASK_PRIORITY_MASK) >> TASK_PRIORITY_OFFSET;
}

static inline int taskGetIndexById(int task_id) {
    return (TASK_INDEX_MASK & task_id) >> TASK_INDEX_OFFSET;
}

// TODO: Implement recycling here
static inline int taskFindFreeTaskTableIndex() {
    return global_next_unique_task_id;
}

void initTaskSystem() {
    global_next_unique_task_id = 1;
    global_current_stack_address = (unsigned int *) TASK_STACK_HIGH;

    for( int i = 1; i < TASK_MAX_TASKS; i++ ) {
        TaskDescriptor *task = &(global_task_table[i]);
        task->id = 0;
        task->parent_id = 0;
        task->sp = NULL;
        task->status = ready;
        task->send_id = NULL;
        task->send_buf = NULL;
        task->recv_buf = NULL;
        task->send_len = 0;
        task->recv_len = 0;
        task->next = NULL;
        for(unsigned j = 0; j < TASK_MAX_NAME_SIZE; j++) {
            task->name[j] = '\0';
        }
        task->cpu_time_used = 0;
        task->originalReceiverId = -1;
    }
}

static void printTask(TaskDescriptor *task) {
    if (task->id == 0) return;

    char *statusName = NULL;

    switch (task->status) {
    case ready:
        statusName = "ready  ";
        break;
    case send_block:
        statusName = "send   ";
        break;
    case receive_block:
        statusName = "receive";
        break;
    case reply_block:
        statusName = "reply  ";
        break;
    default:
        statusName = "?      ";
        return;
    }

    bwprintf(COM2,
        "id:%d\tpr:%d\tst:%s   cpu:%u\r\n",
        taskGetIndex(task),
        taskGetPriority(task),
        statusName,
        task->cpu_time_used);
}

void taskDisplayAll() {
    for(unsigned i = 0; i < TASK_MAX_TASKS; i++) {
        printTask(&global_task_table[i]);
    }
}

unsigned int taskIdleRatio() {
    unsigned int idle_time = 0;
    unsigned int busy_time = 0;
    unsigned int max = 0;
    for(unsigned i = 0; i < TASK_MAX_TASKS; i++) {
        if(global_task_table[i].id == 0) continue;
        if(taskGetPriority(&global_task_table[i]) == PRIORITY_IDLE) {
            idle_time += global_task_table[i].cpu_time_used;
        }
        else {
            busy_time += global_task_table[i].cpu_time_used;
        }
        if(global_task_table[i].cpu_time_used > max) {
            max = global_task_table[i].cpu_time_used;
        }
    }
    if(max > 100000) {
        // reset the cpu_time_used for the next time slice
        for(unsigned i = 0; i < TASK_MAX_TASKS; i++) {
            global_task_table[i].cpu_time_used = 0;
        }
        max = 0;
    }
    return (100 * 100 * idle_time) / (idle_time + busy_time);
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
    unsigned int boundary = (unsigned int)
        (global_current_stack_address - TASK_STACK_SIZE - TASK_TRAP_SIZE);

    if( boundary < TASK_STACK_LOW ){
        bwprintf( COM2, "FATAL: at low stack boundary 0x%x.\n\r", boundary );
        return -3; // stack out of bounds
    }
    // TODO: No recycling tasks ids
    // Once all the TASK_MAX_TASKS been given out, will fail to create new tasks
    int unique_id = global_next_unique_task_id;
    int task_table_index = taskFindFreeTaskTableIndex();
    TaskDescriptor *new_task = &(global_task_table[task_table_index]);
    global_next_unique_task_id++;
    new_task->id = makeId( task_table_index, priority, unique_id );
    new_task->parent_id = parent_id;
    //new_task->ret = 0;
    new_task->sp = global_current_stack_address - TASK_TRAP_SIZE;
    new_task->next = NULL;
    global_current_stack_address -= (TASK_TRAP_SIZE + TASK_STACK_SIZE);

    // init trap frame on stack for c-switch
    *(new_task->sp) = UserMode | DisableFIQ;  // cpsr
    *(new_task->sp + 1) = (unsigned int)code; // pc

    return new_task->id;
}

void taskExit(TaskDescriptor *task) {
    task->id = 0;
    task->parent_id = 0;
    task->sp = NULL;
    task->status = ready;
    task->send_id = NULL;
    task->send_buf = NULL;
    task->recv_buf = NULL;
    task->send_len = 0;
    task->recv_len = 0;
    task->next = NULL;
    for(unsigned j = 0; j < TASK_MAX_NAME_SIZE; j++) {
        task->name[j] = '\0';
    }
    task->cpu_time_used = 0;
    // TODO: task recycling requires re-marking the exited task's stacks as free
}

inline int taskGetMyId(TaskDescriptor *task) {
    return task->id;
}

inline int taskGetMyParentId(TaskDescriptor *task) {
    return task->parent_id;
}

void taskSetRet(TaskDescriptor *task, int ret) {
    *(task->sp + 2) = ret;
}

char *taskGetName(TaskDescriptor *task) {
    return task->name;
}

void taskSetName(TaskDescriptor *task, char *name) {
    strncpy(task->name, name, TASK_MAX_NAME_SIZE);
    task->name[TASK_MAX_NAME_SIZE - 1] = '\0'; // ensure null terminated str
}

TaskDescriptor *taskGetTDByIndex(int index) {
    if (index <= 0 || index >= TASK_MAX_TASKS) {
        return NULL;
    }
    return &(global_task_table[index]);
}

TaskDescriptor *taskGetTDById(int task_id) {
    int index = task_id & TASK_INDEX_MASK;
    if( index <= 0 || index >= TASK_MAX_TASKS ) {
        return NULL;
    }
    return &(global_task_table[index]);
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

void taskForceKill(TaskDescriptor *task) {
    // TODO: Implment killing tasks
}
