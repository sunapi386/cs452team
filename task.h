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
struct TaskDescriptor {
    char name[32];
    int id;
    int parent_id;
    int priority;
    unsigned int sp; // stackpointer
    unsigned int cpsr; // save this with the TD
    struct TaskDescriptor *next; // linked list style

    enum state {
        TASK_READY,
        TASK_ACTIVE,
        TASK_ZOMBIE,
    };

};

/**
Name. Create - instantiate a task.
Synopsis. int Create( int priority, void (*code) ( ) )
Description. Create allocates and initializes a task descriptor, using the given priority, and the given function pointer as a pointer to the entry point of executable code, essentially a function with no arguments and no return value. When Create returns the task descriptor has all the state needed to run the task, the task’s stack has been suitably initialized, and the task has been entered into its ready queue so that it will run the next time it is scheduled.
Returns.
• tid – the positive integer task id of the newly created task. The task id must be unique, in the sense that no task has, will have or has had the same task id.
i• -1 – if the priority is invalid.
• -2 – if the kernel is out of task descriptors.
Do rough tests to ensure that the function pointer argument is valid.
*/
int Create( int priority, void (*code) ( ) ) {
    swi( create );
    // what the little code above this is is doing is moving arguments: taking them from where gcc has put them
    // (*f()) is going to be put into the memory map // we can put a piece of memory as designated of the memory map.
    // moving the return value, taking it where the kernel put it and put it where gcc wants it to be
    // there's no reasonable at all why the kernel can't just pick up and put down the variabels where gcc is going to look for it.
    // gcc looks in r1, r2, r3. and there is one system call that has 5 arguments; 5th argument is the stack
    // gcc is going to put whatever is returned at the end of the computation at r0.
    // the kerne lalso needs to read the

    // save the kernel, install the task, movs

    // what happens when the swi is claled in the middle of this? we run the middle of this. movs (save the user task)< install the kernel.
}

swi(create ) {
    // 1/ the allocation of a task descriptor and as long as the free list of td is not empty, then we can allocate to it. we keep td in arrays because we want constant time operation.
    // 2/ you need some memory. in your initalizaiton of the kernel, you made a decisiooon of how to divide up the memmory; you need a stack to put its memeoyr in
    // 3/ you need a task id. it should be chosen so it is easy to find the td out of the td array.
    // you need the user's stack pointer. so what kinds of things might you put in stack? - register contents (user stack).
    // maybies in the td stack: the ptid, the extra linkregister (lr). could be in the stack or the td. we also need a return value - when task 4 creates priority of task 3
}

/**
Name. MyTid - find my task id.
Synopsis. int MyTid( )
Description. MyTid returns the task id of the calling task.
Returns.
• tid – the positive integer task id of the task that calls it.
• Errors should be impossible!
*/
int MyTid( ) {
    return
}

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