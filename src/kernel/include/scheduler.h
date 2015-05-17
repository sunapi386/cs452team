#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#define TASK_QUEUE_SIZE 128

typedef struct TaskDescriptor TaskDescriptor;

// Task Queue

typedef
struct TaskQueue
{
	int head, tail;
	TaskDescriptor *tasks[TASK_QUEUE_SIZE];
}
TaskQueue;

// Scheduler

void InitScheduler();
TaskDescriptor * Scheuler();
int EnqueueTask(TaskDescriptor *task);

#endif
