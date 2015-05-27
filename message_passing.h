#ifndef __MESSAGE_PASSING_H

struct TaskDescriptor;
struct Syscall;

void initMessagePassing();

void handleSend(struct TaskDescriptor *sendingTask, struct Syscall *request);

void handleReceive(struct TaskDescriptor *receivingTask, struct Syscall *request);

void handleReply(struct TaskDescriptor *receivingTask, struct Syscall *request);

#endif
