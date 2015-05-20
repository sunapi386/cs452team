#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#define NULL 0

typedef struct TaskDescriptor TaskDescriptor;

// Task Queue

typedef
struct TaskQueue
{
    TaskDescriptor *head, *tail;
} TaskQueue;

// Scheduler

void InitScheduler();
TaskDescriptor * Scheuler();
void EnqueueTask(TaskDescriptor *task);

#endif
