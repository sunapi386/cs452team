/** scheduler.h
The task at the front of the highest-priority FIFO queue will be granted the CPU until it terminates or blocks.
The dispatcher uses a 32-level priority scheme to determine the order of task execution. 
*/
#include <scheduler.h>
#include <task.h>

static unsigned int queueStatus = 0;
static TaskQueue taskQueues[32];

/**
Get the position of the least significant set bit position in queueStatus
Credit: http://graphics.stanford.edu/~seander/bithacks.html#IntegerLogDeBruijn
 */
int GetTopPriorityQueueIndex()
{
    static const int MultiplyDeBruijnBitPosition2[32] =
    {
        0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
        31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
    };
    return MultiplyDeBruijnBitPosition2[(unsigned int)((queueStatus ^ (queueStatus & (queueStatus - 1))) * 0x077CB531U) >> 27];
}

/**
    Initialize scheduler data structures
 */
void InitScheduler()
{
    int i;
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
TaskDescriptor * Scheduler()
{
    if (queueStatus == 0)
    {
        return (TaskDescriptor *)0;
    }
    
    // Get the least set 1 bit from queueStatus
    int index = GetTopPriorityQueueIndex();
    
    // Get the task queue with the highest priority
    TaskQueue *q = &taskQueues[index];
    TaskDescriptor *active =  q->head;
    
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
void EnqueueTask(TaskDescriptor *task)
{
    int priority = task->priority;
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
}
