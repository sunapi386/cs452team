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
#define TASK_STACK_SIZE     12000
#define TASK_STACK_HIGH     0x1f00000
#define TASK_STACK_LOW      (TASK_STACK_HIGH - \
                            TASK_MAX_TASKS * (TASK_STACK_SIZE + TASK_TRAP_SIZE))

#define TASK_MAX_NAME_SIZE  20

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

/// Warning! Do not modify the ordering in the TaskDescriptor struct!
/// The context switch assembly file sets these fields. Modifying ordering
/// means wrong fields will be set! (Unless you modify context switching too.)

typedef struct TaskDescriptor {
    int id;
    int parent_id;
    unsigned int *sp;
    enum {
        ready,         // ready to be activated
        send_block,     // task executed Send(), waiting for it to be received
        receive_block, // task executed Receive(), waiting for task to Send()
        reply_block,   // task executed Send(), its message received, waiting on reply
    } status;
    int *send_id;
    void *send_buf, *recv_buf;
    unsigned int send_len, recv_len;
    struct TaskDescriptor *next;
    char name[TASK_MAX_NAME_SIZE];
    unsigned int cpu_time_used;
    int originalReceiverId;
} TaskDescriptor;

int taskCreate(int priority, void (*code)(void), int parent_id);
unsigned int taskIdleRatio();
void initTaskSystem();
void taskDisplayAll();
void taskExit(TaskDescriptor *task);
void taskForceKill(TaskDescriptor *task);
void taskSetName(TaskDescriptor *task, char *name);

/* Returns NULL on invalid task_id */
char *taskGetName(TaskDescriptor *task);
int taskGetIndex(TaskDescriptor *task);
int taskGetIndexById(int task_id);
int taskGetMyId(TaskDescriptor *task);
int taskGetMyParentId(TaskDescriptor *task);
int taskGetMyParentIndex(TaskDescriptor *task);
int taskGetMyParentUnique(TaskDescriptor *task);
int taskGetPriority(TaskDescriptor *task);
int taskGetUnique(TaskDescriptor *task);
TaskDescriptor *taskGetTDById(int task_id);
TaskDescriptor *taskGetTDByIndex(int index);
void taskSetRet(TaskDescriptor *task, int ret);

int isValidTaskIndex(int index);

#endif
