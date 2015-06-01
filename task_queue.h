#ifndef __TASK_QUEUE_H
#define __TASK_QUEUE_H

typedef struct TaskQueue {
    struct TaskDescriptor *head;
    struct TaskDescriptor *tail;
} TaskQueue;

#endif
