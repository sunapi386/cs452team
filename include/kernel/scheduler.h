#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#ifdef KERNEL_MAIN

#include <util.h>
#include <kernel/task.h>
#include <kernel/task_queue.h>

static unsigned int queueStatus = 0;
static TaskQueue readyQueues[32];

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
    if (queueStatus == 0)
    {
        return NULL;
    }

    int index = countLeadingZeroes(queueStatus);

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
