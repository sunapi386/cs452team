#include <scheduler.h>
#include <task.h>
#include <bwio.h>

static unsigned int queueStatus = 0;
static TaskQueue readyQueues[32];

static inline int getQueueIndex()
{
    static const int bitPositions[32] =
    {
        0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
        31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
    };
    return bitPositions[(unsigned int)((queueStatus ^ (queueStatus & (queueStatus - 1))) * 0x077CB531U) >> 27];
}

void initScheduler()
{
    queueStatus = 0;
    int i;
    for (i = 0; i < 32; i++)
    {
        readyQueues[i].tail = NULL;
    }
}

/**
    Called by the kernel; dequeue the first task in the highest priority queue
    Returns:
        Success: Pointer to the TD of the next active task
        Fail: NULL
 */
TaskDescriptor * schedule()
{
    if (queueStatus == 0)
    {
        return NULL;
    }

    // Get the least set 1 bit from queueStatus
    int index = getQueueIndex();

    // Get the task queue with the highest priority
    TaskQueue *q = &readyQueues[index];

    TaskDescriptor *active = q->tail->next;

    if (active == q->tail)
    {
        // if queue becomes empty, set the tail to NULL and
        // clear the status bit
        q->tail = NULL;
        queueStatus &= ~(1 << index);
    }
    else
    {
        q->tail->next = active->next;
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
        q->tail = (TaskDescriptor *)task;
        task->next = (TaskDescriptor *)task;
        queueStatus |= (1 << priority);
    }
    else
    {
        // add the task to the tail
        task->next = q->tail->next;
        q->tail = (TaskDescriptor *)task;
    }
}

