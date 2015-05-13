#ifndef __TASK_H
#define __TASK_H

/**
Task.h
Each task has a task descriptor (TD), which is the most important data structure in the kernel. The TDs should be local to the kernel, that is, on the kernel stack, allocated when the kernel begins executing. Each TD must either include
1. a task identifier unique to this instance of the task,
2. the task’s state,
3. the task’s priority,
4. the task identifier of the task’s parent, the task that created this task,
5. a stack pointer pointing to this task’s private memory,
6. this task’s saved program status register (SPSR), and
7. the task’s return value, which is to be returned to the task when it
next executes, or be able to calculate it using one or more fields of the td. The TD may include other fields. For example, a popular priority queue implementation is a doubly-linked circular list, using pointers to the TDs of tasks ahead and behind the task in the queue that are part of the TD.
A good rule-of-thumb is that values accessed only by the context switch can be on the stack of the user task; other values should be in the task descriptor.*/
struct task {
    int id;
    int state;
    int priority;
    int parent;


};



#endif