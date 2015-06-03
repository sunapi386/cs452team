#ifdef KERNEL_MAIN

#include <kernel/task_queue.h>
#include <kernel/task.h>
#include <user/syscall.h>
#include <utils.h>

extern void queueTask(TaskDescriptor *task);

static TaskQueue sendQueues[TASK_MAX_TASKS];

static inline void initMessagePassing()
{
    int i;
    for (i = 0; i < TASK_MAX_TASKS; i++)
    {
        sendQueues[i].head = NULL;
        sendQueues[i].tail = NULL;
    }
}

static inline void handleSend(TaskDescriptor *sendingTask, Syscall *request)
{
    int tid = (int)request->arg1;
    void *msg = (void *)request->arg2;
    unsigned int msglen = request->arg3;
    void *reply = (void *)request->arg4;
    unsigned int replylen = request->arg5;

    if (!isValidTaskIndex(tid))
    {
        // -1: Task id is impossible
        sendingTask->ret = -1;
        queueTask(sendingTask);
        return;
    }

    TaskDescriptor *receivingTask = taskGetTDByIndex(tid);
    if (!receivingTask)
    {
        // -2: Task id is not an existing task
        sendingTask->ret = -2;
        queueTask(sendingTask);
        return;
    }

    // Since sender cannot Receive() during the transaction, we
    // reuse the receive pointer for getting back the reply later
    sendingTask->recv_buf = reply;
    sendingTask->recv_len = replylen;

    // 1) receiving task is rcv_blk (receive first)
    // copy msg from here to receiving task's stack
    if (receivingTask->status == receive_block)
    {
        //bwprintf(1, "send() case 1: receiver receive_block\n\r");
        // copy message over
        unsigned int copiedLen = msglen < receivingTask->recv_len ?
            msglen : receivingTask->recv_len;
        memcpy(receivingTask->recv_buf, msg, copiedLen);

        // set *tid of sending task
        *(receivingTask->send_id) = taskGetIndex(sendingTask);

        // set return value of receivingTask
        receivingTask->ret = msglen;

        // update sending_task's status to reply_block
        sendingTask->status = reply_block;

        // update receiving_task's status to ready and requeue it
        receivingTask->status = ready;
        queueTask(receivingTask);
    }
    // 2) tid task is not receive_block (send first)
    // queue sending_task to send_queue of receiving_task
    else
    {
        //bwprintf(1, "send() case 2: sender becomes send_block\n\r");
        // get receiving_task's send queue
        TaskQueue *q = &sendQueues[tid];

        if (q->tail == NULL)
        {
            // empty queue
            q->head = sendingTask;
            q->tail = sendingTask;
            sendingTask->next = NULL;
        }
        else
        {
            // non-empty queue
            q->tail->next = sendingTask;
            q->tail = sendingTask;
        }

        sendingTask->send_buf = msg;
        sendingTask->send_len = msglen;
    }
}

static inline void handleReceive(TaskDescriptor *receivingTask, Syscall *request)
{
    int *tid = (int *)request->arg1;
    void *msg = (void *)request->arg2;
    unsigned int msglen = request->arg3;

    TaskQueue *q = &sendQueues[taskGetIndex(receivingTask)];

    // if send queue empty, receive block
    if (q->head == NULL)
    {
        receivingTask->send_id = tid;
        receivingTask->recv_buf = msg;
        receivingTask->recv_len = msglen;
        receivingTask->status = receive_block;
    }
    // if send queue not empty, dequeue first task, copy msg to msg
    else
    {
        // dequeue the first sending task
        TaskDescriptor *sendingTask = q->head;
        q->head = sendingTask->next;
        sendingTask->next = NULL;
        if (q->tail == sendingTask)
        {
            // that was the only task in the queue
            q->tail = NULL;
        }

        // copy data from sending_task's buffer to receiver's buffer
        unsigned int copiedLen = msglen < sendingTask->send_len ?
            msglen : sendingTask->send_len;
        memcpy(msg, sendingTask->send_buf, copiedLen);

        // set tid of sender
        *tid = taskGetIndex(sendingTask);

        // set receive() ret val: the size of the message SENT
        receivingTask->ret = sendingTask->send_len;

        // sender: reply block
        // receiver: ready to run
        sendingTask->status = reply_block;
        receivingTask->status = ready;
        queueTask(receivingTask);
    }
}

static inline void handleReply(TaskDescriptor *receivingTask, Syscall *request)
{
    int tid = (int)request->arg1; // the id of the original sender
    void *reply = (void *)request->arg2; // the reply message
    unsigned int replylen = (unsigned int)request->arg3; // the length of the message

    if (!isValidTaskIndex(tid))
    {
        // -1: The task id is not a a possible task id
        receivingTask->ret = -1;
        queueTask(receivingTask);
        return;
    }

    // get the sender task descriptor from tid
    TaskDescriptor *sendingTask = taskGetTDByIndex(tid);

    if (!sendingTask)
    {
        // -2: The task id is not an existing task
        receivingTask->ret = -2;
        queueTask(receivingTask);
        return;
    }

    // FIXME: we are not checking if replyer is the
    // actual original receiver. But theres a good way...
    else if (sendingTask->status != reply_block)
    {
        // -3: The task is not reply blocked
        receivingTask->ret = -3;
        queueTask(receivingTask);
        return;
    }

    // above in handleSend(), the sender reused the rcv_buffer and rcv_len
    // for getting back the reply

    // copy reply data
    unsigned int copiedLen = replylen > sendingTask->recv_len ?
        sendingTask->recv_len : replylen;
    memcpy(sendingTask->recv_buf, reply, copiedLen);

    // -4: Insufficient space for the entire reply in the sender's reply buffer
    receivingTask->ret = replylen > sendingTask->recv_len ? -4 : 0;

    sendingTask->ret = replylen;

    // set statuses and retvals
    sendingTask->status = ready;

    queueTask(sendingTask);
    queueTask(receivingTask);
}

#endif // KERNEL_MAIN
