#include <task.h>
#include <cpsr.h>

static int global_next_unique_task_id;
static TaskDescriptor *global_active_taskdescriptor;
static TaskDescriptor global_task_table[TASK_MAX_TASKS];

void initTaskSystem() {
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
    // setup first task
    if( taskCreate(1, initalTask, ))
}


int taskCreate(unsigned int priority, void (*code)(void), unsigned int stack_size, int parent_id) {
    if( priority < 0 || priority >= TASK_MAX_PRIORITY || code == NULL ) {
        return -1; // invalid params
    }
    if( next_task_id >= TASK_MAX_TASKS ) {
        return -2; // too many tasks
    }
    if( taskCalcStackHigh(global_next_unique_task_id) < TASK_STACK_LOW ) {
        return -3; // stack out of bounds
    }

    // FIXME:
    // No recycling task ids; once all the task ids been given out, it will fail to create new tasks.
    int new_task_id = taskMakeId();
    TaskDescriptor *new_task = &global_task_table[new_task_id];
    new_task->id = new_task_id;
    new_task->parent_id = parent_id;
    new_task->ret = 0; // FIXME: What should this be, 0?
    new_task->sp = taskMakeSp(new_task_id);
    new_task->spsr = UserMode | DisableIRQ | DIsableFIQ;
    new_task->next = NULL; // FIXME: Check that scheduler takes care of this

    return new_task_id;
}


