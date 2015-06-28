#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#ifdef KERNEL_MAIN

#include <kernel/task.h>
#include <kernel/task_queue.h>
#include <debug.h>

static unsigned int queueStatus = 0;
static TaskQueue readyQueues[32];
// for keeping track of idling usage

static inline void initScheduler()
{
    int i;
    for (i = 0; i < 32; i++)
    {
        readyQueues[i].head = NULL;
        readyQueues[i].tail = NULL;
    }
    queueStatus = 0;
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

    // Get the task queue with the highest priority
    TaskQueue *q = &readyQueues[index];
    TaskDescriptor *active = q->head;

    assert(q->head != NULL);

    if (q->head == q->tail)
    {
        q->head = NULL;
        q->tail = NULL;

        // if queue becomes empty, set the tail to NULL and
        // clear the status bit
        queueStatus &= ~(1 << index);
    }
    else
    {
        q->head = active->next;
        if (q->head == NULL) q->tail = NULL;
    }

    active->next = NULL;

    return active;
}

/**
    Add an active task to the ready queue
 */
void queueTask(TaskDescriptor *task)
{
    int priority = taskGetPriority((TaskDescriptor *)task);
    TaskQueue *q = &readyQueues[priority];

    task->next = NULL;
    if (q->tail == NULL)
    {
        // set up head and tail to the same task; task->next should be NULL;
        // set the bit in queue status to 1 to indicate queue not empty
        q->head = (TaskDescriptor *)task;
        q->tail = (TaskDescriptor *)task;
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
