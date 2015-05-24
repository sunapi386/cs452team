#include <task.h>
#include <cpsr.h>

static int global_next_unique_task_id;
static TaskDescriptor *global_active_taskdescriptor;
static TaskDescriptor global_task_table[TASK_MAX_TASKS];

void initTaskSystem(void (*firstTask)(void)) {
    global_next_unique_task_id = 1;
    global_active_taskdescriptor = NULL;
    global_current_stack = (unsigned int *) TASK_STACK_HIGH;
    for( int i = 1; i < TASK_MAX_TASKS; i++ ) {
        TaskDescriptor *task = global_task_table + i;
        task->id = 0;
        task->parent_id = 0;
        task->ret = 0;
        task->sp = NULL;
        task->spsr = 0;
        task->next = NULL;
    }
    // setup first task, kernel_task
    int ret = taskCreate(1, firstTask, TASK_STACK_SIZE, -1);
    if( ret < 0 ) {
        bwprintf( COM2, "FATAL: initTaskSystem fail creating first task %d.\n\r", ret );
    }
}


int taskCreate(int priority, void (*code)(void), int parent_id) {
    if( priority < 0 || priority >= TASK_MAX_PRIORITY || code == NULL ) {
        return -1; // invalid params
    }
    if( next_task_id >= TASK_MAX_TASKS ) {
        return -2; // too many tasks
    }
    if( taskCalcStackHigh(global_next_unique_task_id) < TASK_STACK_LOW ) {
        return -3; // stack out of bounds
    }

    // FIXME: Jason: Recycle tasks.
    // Once all the task ids been given out, it will fail to create new tasks.
    int task_table_index = taskFindFreeTaskTableIndex();
    int unique_id = global_next_unique_task_id++;
    int new_task_id = taskMakeId( index, priority, unique_id );
    TaskDescriptor *new_task = &global_task_table[new_task_id];
    new_task->id = new_task_id;
    new_task->parent_id = parent_id;
    new_task->ret = 0;
    new_task->sp = taskMakeSp(new_task_id);
    new_task->spsr = UserMode | DisableIRQ | DIsableFIQ;
    new_task->next = NULL; // FIXME: Shuo: Check that scheduler takes care of this

    return new_task_id;
}

int taskFindFreeTaskTableIndex(); // FIXME: Jason: Implement this




// Begin Scheduler Code
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

// End Scheduler Code