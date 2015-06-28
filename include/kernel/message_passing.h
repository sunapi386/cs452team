#ifndef __MESSAGE_PASSING_H
#define __MESSAGE_PASSING_H
struct TaskQueue;
struct TaskDescriptor;
struct Syscall;

void initMessagePassing(struct TaskQueue *sendQueues);
void handleSend(struct TaskQueue *sendQueues, struct TaskDescriptor *sendingTask, struct Syscall *request);
void handleReceive(struct TaskQueue *sendQueues, struct TaskDescriptor *receivingTask, struct Syscall *request);
void handleReply(struct TaskDescriptor *receivingTask, struct Syscall *request);

#endif // __MESSAGE_PASSING_H
