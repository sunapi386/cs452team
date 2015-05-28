#ifndef __SCHEDULER_H
#define __SCHEDULER_H

typedef struct TaskQueue {
    struct TaskDescriptor *tail;
} TaskQueue;

void initScheduler();

/** schedule()
Called by the kernel; dequeue the first task in the highest priority queue
    Returns:
        Success: Pointer to the TD of the next active task
        Fail: NULL
 */
struct TaskDescriptor *schedule();
void queueTask(struct TaskDescriptor *task);
#endif

