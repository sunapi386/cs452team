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
        taskQueues[i].head = 0;
        taskQueues[i].tail = 0;
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
    TaskDescriptor *active =  (TaskDescriptor *)0;
    
    int nextHead = (q->head + 1) % TASK_QUEUE_SIZE;
    int nextTail = (q->tail + 1) % TASK_QUEUE_SIZE;
    if (nextHead != nextTail)
    {
        q->head = nextHead;
        active = q->tasks[nextHead];
        
        // if active task is the last task in the queue, set the status bit to zero
        if ((nextHead + 1) % TASK_QUEUE_SIZE == nextTail)
        {
            queueStatus &= ~(1 << index);
        }
    }
    
    return active;
}

/**
Add an active task to the ready queue
Returns:
    Success: 0
    Fail: -1
 */
int EnqueueTask(TaskDescriptor *task)
{
    int retVal = 0;
    int priority = task->priority;
    TaskQueue *q = &taskQueues[priority];
    
    if ((q->tail + 1) % TASK_QUEUE_SIZE == q->head)
    {
        // overflow: wrap around
        q->tail = q->head;
        retVal = -1;
    }
    
    q->tail = (q->tail + 1) % TASK_QUEUE_SIZE;
    q->tasks[q->tail] = task;
    queueStatus |= 1 << priority;
        
    return retVal;
}
