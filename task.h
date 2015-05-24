#ifndef __TASK_H
#define __TASK_H

// FIXME: Shuo please verify this.
// Trap size is # of registers pushed on stack during c-switch. Layout:
// sp + 0 = r15 (pc)
// sp + 1 = r0
// ...
// sp + 12 = r11 (fp)
// sp + 13 = r12
// sp + 14 = r14 (lr)
#define TASK_TRAP_SIZE      15
#define TASK_BITS           8   // 2^8 = 128
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
#define TASK_STACK_SIZE     1024
#define TASK_STACK_HIGH     0x500000
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

typedef struct TaskDescriptor {
    int id;
    int parent_id;
    int ret;
    unsigned int *sp;
    unsigned int spsr;
    struct TaskDescriptor *next; // linked list through tasks
} TaskDescriptor;


void initTaskSystem();

/* Returns:
   -1: invalid priority or code pointer
   -2: too many tasks created
   -3: no more stack space
   non-negative: newly created task id
 */
int taskCreate(int priority, void (*code)(void), int parent_id);
void taskExit();
void taskSetReturnValue(TaskDescriptor *task, int ret);

int taskGetMyId(TaskDescriptor *task);
int taskGetMyParentId(TaskDescriptor *task);
int taskGetPriority(TaskDescriptor *task);



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

// FIXME
static inline unsigned int *taskMakeSp(int task_id) {
    return 0;
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


#endif
