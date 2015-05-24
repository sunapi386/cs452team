#ifndef __SYSCALL_H
#define __SYSCALL_H

#define SYS_CREATE          0
#define SYS_MY_TID          1
#define SYS_MY_PARENT_TID   2
#define SYS_PASS            3
#define SYS_EXIT            4

typedef struct Syscall {
    unsigned int type, arg1, arg2;
    int ret;
} Syscall;

/**
Name. Create - instantiate a task.
Synopsis. int Create( int priority, void (*code) ( ) )
Description. Create allocates and initializes a task descriptor, using the given priority, and the given function pointer as a pointer to the entry point of executable code, essentially a function with no arguments and no return value. When Create returns the task descriptor has all the state needed to run the task, the task’s stack has been suitably initialized, and the task has been entered into its ready queue so that it will run the next time it is scheduled.
Returns.
    tid: positive integer task id of the newly created task. The task id must be unique, in the sense that no task has, will have or has had the same task id.
    -1 : the priority is invalid.
    -2 : the kernel is out of task descriptors.
Do rough tests to ensure that the function pointer argument is valid.
*/
int Create( int priority, void (*code) ( ) );

/**
Name. MyTid - find my task id.
Synopsis. int MyTid( )
Description. MyTid returns the task id of the calling task.
Returns.
    tid: the positive integer task id of the task that calls it.
*/
int MyTid( );

/**
Name. MyParentTid - find the task id of the task that created the running
task.
Synopsis. int MyParentTid( )
Description. MyParentTid returns the task id of the task that created the calling task.
This will be problematic only if the task has exited or been destroyed, in which case the return value is implementation-dependent.
Returns.
    MyParentTidtid: the task id of the task that created the calling task.
    -1 : if the parent has exited, been destroyed, or is in the process of being destroyed
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
