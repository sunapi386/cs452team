#ifndef __TASK_H
#define __TASK_H

// Trap size is # of registers pushed on stack during c-switch. Layout:
// sp + 0 = r1 (pc)
// sp + 1 = r2 (cpsr_user)
// ...
// sp + 11 = r12
// sp + 12 = lr

#define TASK_TRAP_SIZE      14
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
#define TASK_STACK_SIZE     12000
#define TASK_STACK_HIGH     0x1f00000
#define TASK_STACK_LOW      (TASK_STACK_HIGH - \
                            TASK_MAX_TASKS * (TASK_STACK_SIZE + TASK_TRAP_SIZE))

#define NULL 0


/**
Task.h
Each task has a task descriptor (TD), which is the most important data structure
in the kernel. The TDs should be local to the kernel, that is, on the kernel
stack, allocated when the kernel begins executing. Each TD must either include
1. a task identifier unique to this instance of the task,
2. the task's state,
3. the task's priority,
4. the task identifier of the task's parent, the task that created this task,
5. a stack pointer pointing to this task's private memory, and
6. the task's return value, which is to be returned to the task when it
next executes, or be able to calculate it using one or more fields of the td.
The TD may include other fields. For example, a popular priority queue
implementation is a doubly-linked circular list, using pointers to the TDs of
tasks ahead and behind the task in the queue that are part of the TD.
A good rule-of-thumb is that values accessed only by the context switch can be
on the stack of the user task; other values should be in the task descriptor.
*/

typedef enum {
    ready,         // ready to be activated
    send_block,  // task executed Send(), waiting for it to be received
    receive_block, // task executed Receive(), waiting for task to Send()
    reply_block,   // task executed Send(), its message received, waiting on reply
} Status;

typedef struct TaskDescriptor {
    int id;
    int parent_id;
    int ret;
    unsigned int *sp;
    unsigned int hwi;
    Status status;
    int *send_id;
    void *send_buf, *recv_buf;
    unsigned int send_len, recv_len;
    struct TaskDescriptor *next;
} TaskDescriptor;

int taskCreate(int priority, void (*code)(void), int parent_id);
void initTaskSystem();
void taskSetReturnValue(TaskDescriptor *task, int ret);
int taskGetMyId(TaskDescriptor *task);
int taskGetMyParentId(TaskDescriptor *task);

/* Returns NULL on invalid task_id */
TaskDescriptor *taskGetTDByIndex(int index);
TaskDescriptor *taskGetTDById(int task_id);
int taskGetMyParentIndex(TaskDescriptor *task);
int taskGetUnique(TaskDescriptor *task);
int taskGetMyParentUnique(TaskDescriptor *task);

static inline int isValidTaskIndex(int index)
{
    return index > 0 && index < TASK_MAX_TASKS;
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
