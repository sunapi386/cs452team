#ifndef __SCHEDULER_H
#define TASK_DBRJN_SQN 0x077CB531U
#include <task.h>

typedef struct TaskQueue {
    struct TaskDescriptor *head;
    struct TaskDescriptor **tail;
} TaskQueue;

/** scheduleTask()
Called by the kernel; dequeue the first task in the highest priority queue
    Returns:
        Success: Pointer to the TD of the next active task
        Fail: NULL
 */
void initScheduleSystem();
struct TaskDescriptor *scheduleTask();
#endif
