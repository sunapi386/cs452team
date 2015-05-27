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
#include <message_passing.h>

// receiving task (receive first)
void first()
{
    bwprintf(1, "Receiving task tid: %d\n\r", MyTid());

    int retval = 0;

    char recv_buf[50];
    int tid = -1;
    retval = Receive(&tid, recv_buf, sizeof(recv_buf));
    bwprintf(1, "Receive returned: %d\n\r", retval);
    int i;
    for (i = 0; i < retval; i++)
    {
        bwputc(1, recv_buf[i]);
    }

    bwprintf(1, "\n\r");

    // reply unblocks sender
    char *reply = "k got it";
    int len = strlen(reply) + 1;
    bwprintf(1, "Reply len: %d\n\r", len);

    retval = Reply(tid, reply, len);
    bwprintf(1, "Reply returned: %d\n\r", retval);

    Exit();
}

// sending task (send later)
void second()
{
    bwprintf(1, "Sending task tid: %d\n\r", MyTid());

    int retval = 0;

    char reply[2];
    char *send_buf = "Hw!";
    int len = strlen(send_buf);
    bwprintf(1, "Size of sending reply buffer: %d\n\r", sizeof(reply));
    retval = Send(2, send_buf, len+1, &reply, sizeof(reply));

    bwprintf(1, "Send returned: %d\n\r", retval);

    // Blocked here until second() reply
    bwprintf(1, "Got reply\n\r");
    int i;
    for (i = 0; i<sizeof(reply); i++)
    {
        bwputc(1, reply[i]);
    }

    Exit();
}

void bootstrap()
{
    Create(5, &first);
    Create(6, &second);
    Exit();
}

void initKernel() {
    // Initialize swi jump table to kernel entry point
    *(unsigned int *)(0x28) = (unsigned int)(&KernelEnter);
    initTaskSystem();
    initScheduler();
    initMessagePassing();

    int create_ret = taskCreate(1, &bootstrap, -1);
    if( create_ret < 0 ) {
        bwprintf( COM2, "FATAL: fail creating first task.\n\r" );
        return;
    }
    queueTask(taskGetTDById(create_ret));
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
