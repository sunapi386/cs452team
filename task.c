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

