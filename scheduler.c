#include <scheduler.h>
#include <task.h>
#include <bwio.h>

static unsigned int queueStatus = 0;
static TaskQueue taskQueues[32];

static inline int getQueueIndex()
{
    static const int bitPositions[32] =
    {
        0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
        31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
    };
    return bitPositions[(unsigned int)((queueStatus ^ (queueStatus & (queueStatus - 1))) * 0x077CB531U) >> 27];
}

void initScheduleSystem()
{
    volatile int i;
    for (i = 0; i < 32; i++)
    {
        taskQueues[i].head = NULL;
        taskQueues[i].tail = NULL;
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
        bwprintf(COM2, "Schedule failed, queue status is 0.\n\r");
        return NULL;
    }

    // Get the least set 1 bit from queueStatus
    int index = getQueueIndex();

    // Get the task queue with the highest priority
    TaskQueue *q = &taskQueues[index];
    TaskDescriptor *active = q->head;

    if (active == NULL)
    {
        bwprintf(COM2, "Error: active is NULL but index is returned: %d\n\r", index);
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

    if (active == NULL)
    {
        bwprintf(COM2, "Schedule failed, pri: %d, head: %x, tail: %x\n\r", index, q->head, q->tail);
    }
    else
    {
        bwprintf(COM2, "Schedule successful, pri: %d, head: %x, tail: %x\n\r", index, q->head, q->tail);
    }

    return active;
}

/**
    Add an active task to the ready queue
 */
void queueTask(TaskDescriptor *task)
{
    int priority = taskGetPriority(task);
    TaskQueue *q = &taskQueues[priority];

    if (q->tail == NULL)
    {
        // set up head and tail to the same task; task->next should be NULL;
        // set the bit in queue status to 1 to indicate queue not empty
        q->head = task;
        q->tail = task;
        task->next = NULL;
        queueStatus |= 1 << priority;
    }
    else
    {
        // add the task to the tail
        q->tail->next = task;
        q->tail = task;
    }

    bwprintf(COM2, "Enqueue successful, head: %x, tail: %x\n\r", q->head, q->tail);
}

