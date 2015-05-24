#ifndef __SCHEDULER_H
#define __SCHEDULER_H

#define NULL 0

// Task Queue

typedef
struct TaskQueue
{
    struct TaskDescriptor *head, *tail;
} TaskQueue;

// Scheduler

void initScheduler();
struct TaskDescriptor * scheduler();
void enqueueTask(struct TaskDescriptor *task);

#endif
