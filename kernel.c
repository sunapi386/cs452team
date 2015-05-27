#include <ts7200.h>
#include <stdbool.h>
#include <cpsr.h>
#include <task.h>
#include <syscall.h>
#include <context_switch.h>
#include <scheduler.h>
#include <bwio.h>
#include <user_task.h>
#include <utils.h>

void initKernel() {
    // Initialize swi jump table to kernel entry point
    *(unsigned int *)(0x28) = (unsigned int)(&KernelEnter);
    initTaskSystem();
    initScheduler();

    int create_ret = taskCreate(1, &userModeTask, -1);
    if( create_ret < 0 ) {
        bwprintf( COM2, "FATAL: fail creating first task.\n\r" );
        return;
    }
    queueTask(taskGetTDById(create_ret));
}

static TaskQueue sendQueues[128];

void handleSend(TaskDescriptor *sendingTask, Syscall *request)
{
    int tid = (int)request->arg1;
    void *msg = (void *)request->arg2;
    unsigned int msglen = request->arg3;
    void *reply = (void *)request->arg4;
    unsigned int replylen = request->arg5;

    TaskDescriptor *receivingTask = taskGetTDById(tid);

    // Since sender cannot Receive() during the transaction, we
    // reuse the receive pointer for getting back the reply later
    sendingTask->recv_buf = reply;
    sendingTask->recv_len = replylen;

    // 1) receiving task is rcv_blk (receive first)
    // copy msg from here to receiving task's stack
    if (receivingTask->status == receive_block)
    {
        // copy message over
        unsigned int copiedLen = msglen < receivingTask->recv_len ?
            msglen : receivingTask->recv_len;
        memcpy(receivingTask->recv_buf, msg, copiedLen);

        // set *tid of sending task
        *(receivingTask->send_id) = tid;

        // set return value of receivingTask
        receivingTask->ret = copiedLen;

        // update sending_task's status to reply_block
        sendingTask->status = reply_block;

        // update receiving_task's status to none and requeue it
        receivingTask->status = none;
        queueTask(receivingTask);
    }
    // 2) tid task is not receive_block (send first)
    // queue sending_task to send_queue of receiving_task
    else
    {
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

        // update sending_task status to send_block
        sendingTask->status = send_block;
    }
}

void handleReceive(TaskDescriptor *receivingTask, Syscall *request)
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
        *tid = taskGetUnique(receivingTask);

        // set receive() ret val
        receivingTask->ret = copiedLen;

        // sender: reply block
        // receiver: ready to run
        sendingTask->status = reply_block;
        receivingTask->status = none;
        queueTask(receivingTask);
    }
}

void handleReply(TaskDescriptor *receivingTask, Syscall *request)
{
    int tid = (int)request->arg1; // the id of the original sender
    void *reply = (void *)request->arg2; // the reply message
    unsigned int replylen = (unsigned int)request->arg3; // the length of the message

    if (tid < 0 || tid >= TASK_MAX_TASKS)
    {
        // -1: The task id is not a a possible task id
        receivingTask->ret = -1;
        return;
    }

    // get the sender task descriptor from tid
    TaskDescriptor *sendingTask = taskGetTDByIndex(tid);

    if (!sendingTask)
    {
        // -2: The task id is not an existing task
        receivingTask->ret = -2;
        return;
    }

    // FIXME: we are not checking if replyer is the
    // actual original receiver. But theres a good way...
    else if (sendingTask->status != reply_block)
    {
        // -3: The task is not reply blocked
        receivingTask->ret = -3;
        return;
    }

    // above in handleSend(), the sender reused the rcv_buffer and rcv_len
    // for getting back the reply

    // copy reply data
    unsigned int copiedLen = replylen > sendingTask->recv_len ?
        sendingTask->recv_len : replylen;
    memcpy(sendingTask->recv_buf, reply, copiedLen);

    // unblock both tasks
    sendingTask->status = none;
    receivingTask->status = none;

    queueTask(sendingTask);
    queueTask(receivingTask);
}

void handleRequest(TaskDescriptor *td, Syscall *request) {
    switch (request->type) {
        case SYS_CREATE: {
            int create_ret = taskCreate(request->arg1, (void*)request->arg2, td->parent_id);
            if (create_ret >= 0) {
                td->ret = taskGetIndex(td);
                queueTask(taskGetTDById(create_ret));
            } else {
                td->ret = create_ret;
            }
            break;
        }
        case SYS_MY_TID:
            td->ret = taskGetIndex(td);
            break;
        case SYS_MY_PARENT_TID:
            td->ret = taskGetMyParentIndex(td);
            break;
        case SYS_SEND:
            handleSend(td, request);
            return;
        case SYS_RECEIVE:
            handleReceive(td, request);
            return;
        case SYS_REPLY:
            handleReply(td, request);
            return;
        case SYS_PASS:
            break;
        case SYS_EXIT:
            return;
        default:
            bwprintf(COM2, "Invalid syscall %u!", request->type);
            break;
    }
    // requeue the task if we haven't returned (from SYS_EXIT)
    queueTask(td);
}

int main() {
    initKernel();
    volatile TaskDescriptor *task = NULL;

    for(task = schedule() ; ; task = schedule()) {
        Syscall **request = NULL;
        if (task == NULL) {
            bwprintf(COM2, "No tasks scheduled; exiting...\n\r");
            break;
        }

        KernelExit(task, request);
        handleRequest((TaskDescriptor *)task, *request);
    }

    return 0;
}
