#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#ifdef KERNEL_MAIN

#include <kernel/task.h>
#include <kernel/task_queue.h>

static unsigned int queueStatus = 0;
static TaskQueue readyQueues[32];
// for keeping track of idling usage
static unsigned int idling, busy;

static inline void initScheduler()
{
    int i;
    idling = busy = 1; // avoid div by 0
    for (i = 0; i < 32; i++)
    {
        readyQueues[i].head = NULL;
        readyQueues[i].tail = NULL;
    }
    queueStatus = 0;
}
#define MAX_INT_32 2147483647
static inline unsigned int getIdlingRatio() {
    if(idling > MAX_INT_32) { busy = idling = 1; } // reset
    return busy / idling;
}

static inline TaskDescriptor * schedule()
{
    static const int table[32] = {
        0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
        31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
    };

    if (queueStatus == 0)
    {
        return NULL;
    }

    int index = table[(unsigned int)((queueStatus ^ (queueStatus &
        (queueStatus - 1))) * 0x077cb531u) >> 27];

    if(index == 31) { idling++; }
    else { busy++; }

    // Get the task queue with the highest priority
    TaskQueue *q = &readyQueues[index];
    TaskDescriptor *active = q->head;

    if (q->head == NULL &&
        queueStatus == 0)
    {
        return NULL;
    }

    q->head = active->next;

    if (q->head == NULL)
    {
        // if queue becomes empty, set the tail to NULL and
        // clear the status bit
        q->tail = NULL;
        queueStatus &= ~(1 << index);
    }
    else
    {
        active->next = NULL;
    }

    return active;
}

/**
    Add an active task to the ready queue
 */
void queueTask(TaskDescriptor *task)
{
    int priority = taskGetPriority((TaskDescriptor *)task);
    TaskQueue *q = &readyQueues[priority];

    if (q->tail == NULL)
    {
        // set up head and tail to the same task; task->next should be NULL;
        // set the bit in queue status to 1 to indicate queue not empty
        q->head = (TaskDescriptor *)task;
        q->tail = (TaskDescriptor *)task;
        task->next = NULL;
        queueStatus |= 1 << priority;
    }
    else
    {
        // add the task to the tail
        q->tail->next = (TaskDescriptor *)task;
        q->tail = (TaskDescriptor *)task;
    }
}
#endif // KERNEL_MAIN

#endif
