#include <kernel/task_queue.h>
#include <kernel/task.h>
#include <user/syscall.h>
#include <utils.h>
#include <debug.h>

extern void queueTask(TaskDescriptor *task);

void initMessagePassing(TaskQueue *sendQueues)
{
    int i;
    for (i = 0; i < TASK_MAX_TASKS; i++)
    {
        sendQueues[i].head = NULL;
        sendQueues[i].tail = NULL;
    }
}

void handleSend(TaskQueue *sendQueues, TaskDescriptor *sendingTask, Syscall *request)
{
    int tid = (int)request->arg1;
    void *msg = (void *)request->arg2;
    unsigned int msglen = request->arg3;
    void *reply = (void *)request->arg4;
    unsigned int replylen = request->arg5;

    // debug("To: %d, msglen: %d, replylen: %d", tid, msglen, replylen);

    if (!isValidTaskIndex(tid))
    {
        // -1: Task id is impossible
        //sendingTask->ret = -1;
        taskSetRet(sendingTask, -1);
        queueTask(sendingTask);
        return;
    }

    TaskDescriptor *receivingTask = taskGetTDByIndex(tid);
    if (!receivingTask)
    {
        // -2: Task id is not an existing task
        //sendingTask->ret = -2;
        taskSetRet(sendingTask, -2);
        queueTask(sendingTask);
        return;
    }

    // Since sender cannot Receive() during the transaction, we
    // reuse the receive pointer for getting back the reply later
    sendingTask->recv_buf = reply;
    sendingTask->recv_len = replylen;
    sendingTask->next = NULL;

    // 1) receiving task is rcv_blk (receive first)
    // copy msg from here to receiving task's stack
    if (receivingTask->status == receive_block)
    {
        // copy message over
        unsigned int copiedLen = msglen < receivingTask->recv_len ?
            msglen : receivingTask->recv_len;
        memcpy(receivingTask->recv_buf, msg, copiedLen);

        // set *tid of sending task
        *(receivingTask->send_id) = taskGetIndex(sendingTask);

        // set return value of receivingTask
        taskSetRet(receivingTask, msglen);

        receivingTask->recv_buf = NULL;
        receivingTask->recv_len = 0;
        receivingTask->send_id = 0;

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
        assert(sendingTask->status != send_block);
        sendingTask->status = send_block;

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

void handleReceive(TaskQueue *sendQueues, TaskDescriptor *receivingTask, Syscall *request)
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
    // if send queue not empty, dequeue first task, copy send_buf to msg
    else
    {
        // dequeue the first sending task
        TaskDescriptor *sendingTask = q->head;
        if (q->tail == sendingTask)
        {
            // that was the only task in the queue
            q->head = NULL;
            q->tail = NULL;
        }
        else
        {
            q->head = sendingTask->next;
        }

        sendingTask->next = NULL;

        assert(sendingTask->status == send_block);

        // copy data from sending_task's buffer to receiver's buffer
        unsigned int copiedLen = msglen < sendingTask->send_len ?
            msglen : sendingTask->send_len;
        memcpy(msg, sendingTask->send_buf, copiedLen);

        // set tid of senderP
        *tid = taskGetIndex(sendingTask);

        // ret val of receive: the size of the message SENT
        //receivingTask->ret = sendingTask->send_len;
        taskSetRet(receivingTask, sendingTask->send_len);

        // Clean up send buf and length
        sendingTask->send_len = 0;
        sendingTask->send_buf = NULL;

        // sender: reply block
        // receiver: ready to run
        sendingTask->status = reply_block;
        receivingTask->status = ready;
        queueTask(receivingTask);
    }
}

void handleReply(TaskDescriptor *receivingTask, Syscall *request)
{
    // assert(receivingTask != NULL);
    // assert(request != NULL);

    int tid = (int)request->arg1; // the id of the original sender
    void *reply = (void *)request->arg2; // the reply message
    unsigned int replylen = (unsigned int)request->arg3; // the length of the message

    if (!isValidTaskIndex(tid))
    {
        // -1: The task id is not a a possible task id
        //receivingTask->ret = -1;
        taskSetRet(receivingTask, -1);
        queueTask(receivingTask);
        return;
    }

    // get the sender task descriptor from tid
    TaskDescriptor *sendingTask = taskGetTDByIndex(tid);

    if (!sendingTask)
    {
        // -2: The task id is not an existing task
        //receivingTask->ret = -2;
        taskSetRet(receivingTask, -2);
        queueTask(receivingTask);
        return;
    }

    // FIXME: we are not checking if replyer is the
    // actual original receiver. But theres a good way...
    else if (sendingTask->status != reply_block)
    {
        // -3: The task is not reply blocked
        //receivingTask->ret = -3;
        taskSetRet(receivingTask, -3);
        queueTask(receivingTask);
        return;
    }

    // above in handleSend(), the sender reused the rcv_buffer and rcv_len
    // for getting back the reply

    // copy reply data
    unsigned int copiedLen = sendingTask->recv_len < replylen ?
        sendingTask->recv_len : replylen;
    memcpy(sendingTask->recv_buf, reply, copiedLen);

    // -4: Insufficient space for the entire reply in the sender's reply buffer
    //receivingTask->ret = ;
    taskSetRet(receivingTask, replylen > sendingTask->recv_len ? -4 : 0);
    taskSetRet(sendingTask, replylen);

    //sendingTask->ret = replylen;

    sendingTask->recv_len = 0;
    sendingTask->recv_buf = NULL;

    // set statuses and retvals
    receivingTask->status = ready;
    sendingTask->status = ready;
    queueTask(sendingTask);
    queueTask(receivingTask);
}
