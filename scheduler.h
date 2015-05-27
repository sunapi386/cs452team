#ifndef __SCHEDULER_H
#define __SCHEDULER_H

typedef struct TaskQueue {
    struct TaskDescriptor *head;
    struct TaskDescriptor *tail;
} TaskQueue;

void initScheduler();

/** schedule()
Called by the kernel; dequeue the first task in the highest priority queue
    Returns:
        Success: Pointer to the TD of the next active task
        Fail: NULL
 */
volatile struct TaskDescriptor *schedule();
void queueTask(volatile struct TaskDescriptor *task);
#endif

