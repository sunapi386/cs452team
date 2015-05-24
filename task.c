#include <task.h>
#include <cpsr.h>
#include <bwio.h>

static int global_next_unique_task_id;
static unsigned int *global_current_stack;
static TaskDescriptor *global_active_kernel_task;
static TaskDescriptor global_task_table[TASK_MAX_TASKS];
static unsigned int global_queue_status;
static TaskQueue global_task_queues[32];
static int global_debruijn_lookup[32];

void initTaskSystem(void (*firstTask)(void)) {
    global_next_unique_task_id = 1;
    global_active_kernel_task = NULL;
    global_current_stack = (unsigned int *) TASK_STACK_HIGH;
    global_queue_status = 0;
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
    int ret = taskCreate(1, firstTask, -1);
    if( ret < 0 ) {
        bwprintf( COM2, "FATAL: initTaskSystem fail creating first task %d.\n\r", ret );
    }
    for( int i = 0; i < TASK_MAX_PRIORITY; i++ ) {
        global_task_queues[i].head = NULL;
        global_task_queues[i].tail = &global_task_queues[i].head;
    }
    for( int i = 0; i < TASK_MAX_PRIORITY; i++ ) {
        global_debruijn_lookup[ ((1 << i) * TASK_DBRJN_SQN) >> 27 ] = i;
    }

}


int taskCreate(int priority, void (*code)(void), int parent_id) {
    if( priority < 0 || priority >= TASK_MAX_PRIORITY || code == NULL ) {
        return -1; // invalid params
    }
    if( global_next_unique_task_id >= TASK_MAX_TASKS ) {
        return -2; // too many tasks
    }
    if( taskCalcStackHigh(global_next_unique_task_id) < TASK_STACK_LOW ||
        taskCalcStackLow(global_next_unique_task_id) > TASK_STACK_HIGH ) {
        return -3; // stack out of bounds
    }

    // FIXME: Jason: Recycle tasks.
    // Once all the task ids been given out, it will fail to create new tasks.
    int task_table_index = taskFindFreeTaskTableIndex();
    int unique_id = global_next_unique_task_id++;
    int new_task_id = taskMakeId( task_table_index, priority, unique_id );
    TaskDescriptor *new_task = &global_task_table[task_table_index];
    new_task->id = new_task_id;
    new_task->parent_id = parent_id;
    new_task->ret = 0;
    new_task->sp = taskMakeSp(new_task_id);
    new_task->spsr = UserMode | DisableIRQ | DisableFIQ;
    new_task->next = NULL; // FIXME: Shuo: Check that scheduler takes care of this

    return new_task_id;
}

int taskFindFreeTaskTableIndex(); // FIXME: Jason: Implement this

void taskExit() {
    global_active_kernel_task = NULL;
}


// Begin Scheduler Code
/** scheduler.h
The task at the front of the highest-priority FIFO queue will be granted the CPU until it terminates or blocks.
The dispatcher uses a 32-level priority scheme to determine the order of task execution.
*/

/**
Get the position of the least significant set bit position in global_queue_status
Credit: http://graphics.stanford.edu/~seander/bithacks.html#IntegerLogDeBruijn
 */
static inline int highestPQIndex() {
    return global_debruijn_lookup[ ((global_queue_status ^ (global_queue_status & (global_queue_status - 1))) * TASK_DBRJN_SQN) >> 27 ];
}

static inline void taskQueuePush(TaskDescriptor *task) {
    int priority = taskGetPriority(task);
    TaskQueue *q = &global_task_queues[priority];
    *(q->tail) = task; // head <- task
    q->tail &= (q->next); // same as t->next == NULL ? NULL : t->next
    global_queue_status |= 1 << priority;
}

static inline TaskDescriptor *taskQueuePop(int priority) {
    TaskQueue *q = &global_task_queues[priority];
    TaskDescriptor *active =  q->head;
    q->head = active->next;
    active->next = NULL;
    if( ! q->head ) {
        // queue empty, set the tail to NULL, clear the status bit
        q->tail = &q->head;
        global_queue_status &= ~(1 << priority);
    }
    return active;
}

TaskDescriptor *scheduleTask() {
    if( global_active_kernel_task ) {
        taskQueuePush( global_active_kernel_task );
    }
    if( global_queue_status ) {
        return global_active_kernel_task = taskQueuePop( highestPQIndex() );
    }
    return NULL;
}

// End Scheduler Code
