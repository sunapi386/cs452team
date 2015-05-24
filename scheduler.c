#include <scheduler.h>
#include <task.h>
static TaskDescriptor *global_active_kernel_task;
static unsigned int global_queue_status;
static TaskQueue global_task_queues[32];
static int global_debruijn_lookup[32];

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
    q->tail = &(task->next); // same as t->next == NULL ? NULL : t->next
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

void initScheduleSystem() {
    global_active_kernel_task = NULL;
    global_queue_status = 0;
    for( int i = 0; i < TASK_MAX_PRIORITY; i++ ) {
        global_task_queues[i].head = NULL;
        global_task_queues[i].tail = &global_task_queues[i].head;
    }
    for( int i = 0; i < TASK_MAX_PRIORITY; i++ ) {
        global_debruijn_lookup[ ((1 << i) * TASK_DBRJN_SQN) >> 27 ] = i;
    }

}
