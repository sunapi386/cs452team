#ifndef __TASK_H
#define __TASK_H

/**
Task.h
Each task has a task descriptor (TD), which is the most important data structure in the kernel. The TDs should be local to the kernel, that is, on the kernel stack, allocated when the kernel begins executing. Each TD must either include
1. a task identifier unique to this instance of the task,
2. the task’s state,
3. the task’s priority,
4. the task identifier of the task’s parent, the task that created this task,
5. a stack pointer pointing to this task’s private memory,
6. this task’s saved program status register (SPSR), and
7. the task’s return value, which is to be returned to the task when it
next executes, or be able to calculate it using one or more fields of the td. The TD may include other fields. For example, a popular priority queue implementation is a doubly-linked circular list, using pointers to the TDs of tasks ahead and behind the task in the queue that are part of the TD.
A good rule-of-thumb is that values accessed only by the context switch can be on the stack of the user task; other values should be in the task descriptor.*/
typedef stack_t int*;
struct TaskDescriptor {
    char name[32];
    int id;
    int state;
    int parent_id;
    int priority;

    // stack_t *sp; // task's stack
    // cpsr; // save this with the TD
    struct TaskDescriptor *next; // linked list style

    enum status {
        TASK_READY,
        TASK_BLOCKED,
        TASK_ZOMBIE,
    };

};

/**
Name. Create - instantiate a task.
Synopsis. int Create( int priority, void (*code) ( ) )
Description. Create allocates and initializes a task descriptor, using the given priority, and the given function pointer as a pointer to the entry point of executable code, essentially a function with no arguments and no return value. When Create returns the task descriptor has all the state needed to run the task, the task’s stack has been suitably initialized, and the task has been entered into its ready queue so that it will run the next time it is scheduled.
Returns.
• tid – the positive integer task id of the newly created task. The task id must be unique, in the sense that no task has, will have or has had the same task id.
• -1 – if the priority is invalid.
• -2 – if the kernel is out of task descriptors.
Do rough tests to ensure that the function pointer argument is valid.
*/
int Create( int priority, void (*code) ( ) );

/**
Name. MyTid - find my task id.
Synopsis. int MyTid( )
Description. MyTid returns the task id of the calling task.
Returns.
• tid – the positive integer task id of the task that calls it.
• Errors should be impossible!
*/
int MyTid( );

/**
Name. MyParentTid - find the task id of the task that created the running
task.
Synopsis. int MyParentTid( )
Description. MyParentTid returns the task id of the task that created the calling task.
This will be problematic only if the task has exited or been destroyed, in which case the return value is implementation-dependent.
Returns.
• tid – the task id of the task that created the calling task.
• The return value is implementation-dependent if the parent has
exited, has been destroyed, or is in the process of being destroyed.
*/
int MyParentTid( );

/**
Name. Pass - cease execution, remaining ready to run.
Synopsis. void Pass( )
Description. Pass causes a task to stop executing. The task is moved to the end of its priority queue, and will resume executing when next scheduled.
*/
void Pass( );

/**
Name. Exit - terminate execution forever. Synopsis. void Exit( )
Description. Exit causes a task to cease execution permanently. It is removed from all priority queues, send queues, receive queues and awaitEvent queues. Resources owned by the task, primarily its memory and task descriptor are not reclaimed.
Returns. Exit does not return. If a point occurs where all tasks have exited the kernel should return cleanly to RedBoot.
*/
void Exit( );

#endif
