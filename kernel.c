#include <ts7200.h>
#include <stdbool.h>
#include <cpsr.h>
#include <task.h>
#include <syscall.h>
#include <context_switch.h>
#include <scheduler.h>
#include <bwio.h>
#include <user_task.h>

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

// NOTE: Insecure (does not detect overlapping memory)
void memcpy(void *dest, const void *src, unsigned int n)
{
    for (int i = 0; i < n; i++)
    {
        *(unsigned char *)dest = *(unsigned char *)src;
        ++dest;
        ++src;
    }
}

void handleSend(TaskDescriptor *sending_task, Syscall *request)
{
    int tid = (int)request->arg1;
    void *msg = (void *)request->arg2;
    unsigned int msglen = request->arg3;
    void *reply = (void *)request->arg4;
    unsigned int replylen = request->arg5;

    TaskDescriptor *receiving_task = taskGetTDById(tid);

    //1) receiving task is rcv_blk ( first)
    // copy msg from here to receiving task's stack
    if (receiving_task->status == receive_block)
    {
        // memcpy(dst, src, len)
        memcpy(receiving_task->receive,
               msg,
               msglen < receiving_task->receive_len ? msglen : receiving_task->receive_len);


        // set *tid of sending task
        *(receiving_task->sender_id) = tid;

        // update sending_task's status to reply_block
        sending_task->status = reply_block;

        // update receiving_task's status to none and requeue it
        receiving_task->status = none;
        queueTask(receiving_task);
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
            q->head = sending_task;
            q->tail = sending_task;
            sending_task->send_next = NULL;
        }
        else
        {
            // non-empty queue
            q->tail->send_next = sending_task;
            q->tail = sending_task;
        }

        sending_task->send = msg;
        sending_task->send_len = msglen;

        // update sending_task status to send_block
        sending_task->status = send_block;
    }
}

void handleReceive(TaskDescriptor *receiving_task, Syscall *request)
{
    int *tid = (int *)request->arg1;
    void *msg = (void *)request->arg2;
    unsigned int msglen = request->arg3;

    TaskQueue *q = &sendQueues[taskGetIndex(receiving_task)];

    // if send queue empty, receive block
    if (q->head == NULL)
    {
        receiving_task->sender_id = tid;
        receiving_task->receive = msg;
        receiving_task->receive_len = msglen;
        receiving_task->status = receive_block;
    }
    // if send queue not empty, dequeue first task, copy msg to msg
    else
    {
        // dequeue the first sending task
        TaskDescriptor *sending_task = q->head;
        q->head = sending_task->send_next;
        sending_task->send_next = NULL;
        if (q->tail == sending_task)
        {
            // that was the only task in the queue
            q->tail = NULL;
        }

        // copy data from sending_task's buffer to receiver's buffer
        memcpy(msg,
               sending_task->send,
               msglen < sending_task->send_len ? msglen : sending_task->send_len);

        // set tid of sender
        *tid = taskGetUnique(receiving_task);

        // sender: reply block
        // receiver: ready to run
        sending_task->status = reply_block;
        receiving_task->status = none;
        queueTask(receiving_task);
    }
}

void handleReply()
{

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
            break;
        case SYS_RECEIVE:
            handleReceive(td, request);
            break;
        case SYS_REPLY:
            break;
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
