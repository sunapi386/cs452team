#ifndef __TASK_H
#define __TASK_H

#define TASK_TRAP_SIZE      15
#define TASK_BITS           7   // 2^8 = 128
#define TASK_PRIORITY_BITS  5   // 2^5 = 32  Warning: Brujin table is 32.

#define TASK_MAX_TASKS      (1<<TASK_BITS)
#define TASK_MAX_PRIORITY   (1<<TASK_PRIORITY_BITS)
#define TASK_MAX_UNIQUES    (1<<TASK_UNIQUE_BITS)

#define TASK_INDEX_OFFSET       0
#define TASK_PRIORITY_OFFSET    TASK_BITS
#define TASK_UNIQUE_OFFSET      (TASK_BITS + TASK_PRIORITY_BITS)

#define TASK_UNIQUE_BITS    (32 - TASK_UNIQUE_OFFSET)

#define TASK_INDEX_MASK     ((TASK_MAX_TASKS - 1) << TASK_INDEX_OFFSET)
#define TASK_PRIORITY_MASK  ((TASK_MAX_PRIORITY - 1) << TASK_PRIORITY_OFFSET)
#define TASK_UNIQUE_MASK    ((TASK_MAX_UNIQUES - 1) << TASK_UNIQUE_OFFSET)

// each task has own trapframe and stack
#define TASK_STACK_SIZE     TASK_MAX_TASKS * 1024
#define TASK_STACK_HIGH     0x1f00000
#define TASK_STACK_LOW      (TASK_STACK_HIGH - \
                            TASK_MAX_TASKS * (TASK_STACK_SIZE + TASK_TRAP_SIZE))

#define TASK_MAX_NAME_SIZE  20

#define NULL 0

typedef enum {
    ready,         // ready to be activated
    send_block,    // task executed Send(), waiting for it to be received
    receive_block, // task executed Receive(), waiting for task to Send()
    reply_block,   // task executed Send(), its message received, waiting on reply
} Status;

typedef struct TaskDescriptor {
    int id;
    int parent_id;
    unsigned int *sp;
    Status status;
    int *send_id;
    void *send_buf, *recv_buf;
    unsigned int send_len, recv_len;
    struct TaskDescriptor *next;
    char name[TASK_MAX_NAME_SIZE];
    unsigned int cpu_time_used;
    int originalReceiverId;
} TaskDescriptor;

int taskCreate(int priority, void (*code)(void), int parent_id);
TaskDescriptor* taskSpawn(int priority, void (*code)(void), void *argument, int parentId);
void taskExit(TaskDescriptor *task);
void initTaskSystem();
void taskSetName(TaskDescriptor *task, char *name);
int taskGetMyId(TaskDescriptor *task);
int taskGetMyParentId(TaskDescriptor *task);
void taskDisplayAll();
unsigned int taskIdleRatio();

/* Returns NULL on invalid task_id */
TaskDescriptor *taskGetTDByIndex(int index);
TaskDescriptor *taskGetTDById(int task_id);
int taskGetMyParentIndex(TaskDescriptor *task);
int taskGetUnique(TaskDescriptor *task);
int taskGetMyParentUnique(TaskDescriptor *task);
void taskSetRet(TaskDescriptor *task, int ret);
char *taskGetName(TaskDescriptor *task);

static inline int isValidTaskIndex(int index)
{
    return (index > 0) && (index < TASK_MAX_TASKS);
}

// Make sure 0 <= {index,priority,unique} < TASK_{,PRIORITY,UNIQUE}_BITS
static inline int makeId(int index, int priority, int unique) {
    return
        (index << TASK_INDEX_OFFSET) |
        (priority << TASK_PRIORITY_OFFSET) |
        (unique << TASK_UNIQUE_OFFSET);
}

static inline int taskGetIndexById(int task_id) {
    return (TASK_INDEX_MASK & task_id) >> TASK_INDEX_OFFSET;
}

static inline int taskGetIndex(TaskDescriptor *task) {
    return (TASK_INDEX_MASK & task->id) >> TASK_INDEX_OFFSET;
}

static inline int taskGetPriority(TaskDescriptor *task) {
    return (task->id & TASK_PRIORITY_MASK) >> TASK_PRIORITY_OFFSET;
}


#endif
