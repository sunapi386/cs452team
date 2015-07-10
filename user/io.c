#include <user/io.h>
#include <user/syscall.h>
#include <events.h>
#include <utils.h>
#include <priority.h>
#include <ts7200.h>
#include <kernel/task.h> // TASK_MAX_TASKS

#define MONITOR_BUFFER_SIZE 2048
#define TRAIN_BUFFER_SIZE   512

typedef
struct IOReq {
    enum {
        NOTIFICATION,
        PUTCHAR,
        PUTSTR,
        GETCHAR,
        ECHO,
    } type;
    unsigned int data;
} IOReq;

static int com1RecvSrvTid = -1;
static int com1SendSrvTid = -1;
static int com2RecvSrvTid = -1;
static int com2SendSrvTid = -1;

int Getc(int channel) {
    if (channel != COM1 && channel != COM2) return -1;
    IOReq req = { .type = GETCHAR };
    int server_tid = (channel == COM1) ? com1RecvSrvTid : com2RecvSrvTid;
    char c = 0;
    int ret = Send(server_tid, &req, sizeof(req), &c, sizeof(c));
    return ret < 0 ? ret : c;
}

int Putc(int channel, char c) {
    if (channel != COM1 && channel != COM2) return -1;
    IOReq req = { .type = PUTCHAR, .data = (unsigned int) c};
    int server_tid = (channel == COM1) ? com1SendSrvTid : com2SendSrvTid;
    int ret = Send(server_tid, &req, sizeof(req), 0, 0);
    return (ret < 0) ? -1 : 0;
}

int PutString(int channel, String *s) {
    s->buf[s->len] = '\0';
    return PutStr(channel, s->buf);
}

int PutStr(int channel, char *str) {
    if (channel != COM1 && channel != COM2) return -1;
    IOReq req = { .type = PUTSTR, .data = (unsigned int)str };
    int server_tid = (channel == COM1) ? com1SendSrvTid : com2SendSrvTid;
    int ret = Send(server_tid, &req, sizeof(req), 0, 0);
    return (ret < 0) ? -1 : 0;
}

static void monitorInNotifier() {
    int pid = MyParentTid();
    IOReq req = { .type = NOTIFICATION  };
    for (;;) {
        req.data = (unsigned int)AwaitEvent(UART2_RECV_EVENT);
        Send(pid, &req, sizeof(req), 0, 0);
    }
}

static void echoCourier() {
    IOReq echoReq = { .type = ECHO };
    IOReq sendReq = { .type = PUTCHAR };

    for (;;) {
        Send(com2RecvSrvTid, &echoReq, sizeof(echoReq), &(sendReq.data), sizeof(sendReq.data));
        Send(com2SendSrvTid, &sendReq, sizeof(sendReq), 0, 0);
    }
}

static void monitorInServer() {
    char c = 0;
    int tid = 0, courierTid = 0;
    char taskb[TASK_MAX_TASKS];
    char charb[MONITOR_BUFFER_SIZE];
    CBuffer taskBuffer;
    CBuffer charBuffer;
    CBufferInit(&taskBuffer, taskb, TASK_MAX_TASKS);
    CBufferInit(&charBuffer, charb, MONITOR_BUFFER_SIZE);
    IOReq req = { .type = -1, .data = '\0' };
    com2RecvSrvTid = MyTid();
    RegisterAs("monitorInServer");

    Create(PRIORITY_NOTIFIER, &echoCourier);
    Create(PRIORITY_NOTIFIER, &monitorInNotifier);

    for (;;) {
        Receive(&tid, &req, sizeof(req));

        switch (req.type) {
        // the notifier sent us a character
        case NOTIFICATION: {
            // unblock notifier
            Reply(tid, 0, 0);

            if (courierTid)
            {
                // unblock the courier and give it a
                //  char to echo it on the terminal
                Reply(courierTid, &(req.data), sizeof(req.data));
                courierTid = 0;
            }

            c = (char)req.data;
            if(CBufferIsEmpty(&taskBuffer)) {
                // no tasks waiting, put char in a buffer
                CBufferPush(&charBuffer, c);
            }
            else {
                // there are tasks waiting for a char
                // unblock a task and hand it the char
                // that we'd just acquired
                int popped_tid = (int)CBufferPop(&taskBuffer);

                Reply(popped_tid, &c, sizeof(c));
            }
            break;
        }
        // some user task called getchar
        case GETCHAR: {
            if(CBufferIsEmpty(&charBuffer)) {
                // no char to give back to caller; block it
                CBufferPush(&taskBuffer, (char)tid);
            }
            else {
                // characters in the buffer, return it
                char ch = CBufferPop(&charBuffer);
                Reply(tid, &ch, sizeof(ch));
            }
            break;
        }
        // echo wants a character, block it until the next notification
        case ECHO: {
            // block the echoer
            courierTid = tid;
            break;
        }
        default:
            break;
        }
    }
}

static void monitorOutNotifier() {
    int pid = MyParentTid();
    IOReq req = { .type = NOTIFICATION };

    for (;;) {
        req.data = AwaitEvent(UART2_XMIT_EVENT);
        Send(pid, &req, sizeof(req), 0, 0);
    }
}

static void monitorOutServer() {
    int tid = 0;
    char * sendAddr = 0;
    char charb[MONITOR_BUFFER_SIZE];
    CBuffer charBuffer;
    CBufferInit(&charBuffer, charb, sizeof(charb));
    IOReq req = { .type = -1, .data = '\0' };
    com2SendSrvTid = MyTid();
    RegisterAs("monitorOutServer");

    int notifierTid = Create(PRIORITY_MONITOR_OUT_NOTIFIER, &monitorOutNotifier);

    for (;;) {
        Receive(&tid, &req, sizeof(req));

        switch (req.type) {
        case NOTIFICATION: {
            if(CBufferIsEmpty(&charBuffer)) {
                // nothing to send
                // mark that we've seen a xmit interrupt
                // and block the notifier
                sendAddr = (char *)(req.data);
            }
            else {
                char ch = CBufferPop(&charBuffer);
                //assert(req.data == UART2_BASE + UART_DATA_OFFSET);
                // write out char to volatile address
                // this also clears the xmit interrupt
                *((char *)req.data) = ch;

                // unblock the notifier
                Reply(tid, 0, 0);
            }
            break;
        }
        case PUTCHAR: {
            // We've seen a xmit but did not
            // have a char to send at the moment
            if (sendAddr != 0) {
                // send the char directly
                // also clears the xmit interrupt
                *sendAddr = (char)(req.data);

                // unblock the notifier which will
                // call AwaitEvent() which re-enables
                // xmit interrupt
                Reply(notifierTid, 0, 0);

                // reset 'seen transmit int' state
                sendAddr = 0;
            }
            // Have not seen a xmit; just buffer that shit
            else {
                CBufferPush(&charBuffer, (char)(req.data));
            }

            Reply(tid, 0, 0);

            break;
        }
        case PUTSTR: {
            // unblock caller
            Reply(tid, 0, 0);

            // get the ptr to str
            char *str = (char *)(req.data);

            // push entire string into buffer
            CBufferPushStr(&charBuffer, str);

            // unblock notifier we've previously
            // got a xmit but had nothing to send
            if (!CBufferIsEmpty(&charBuffer) && sendAddr != 0) {
                *sendAddr = CBufferPop(&charBuffer);

                // unblock the notifier which will
                // call AwaitEvent() which re-enables
                // xmit interrupt
                Reply(notifierTid, 0, 0);

                // reset 'seen transmit int' state
                sendAddr = 0;
            }
            break;
        }
        default:
            break;
        }
    }
}

static void trainInNotifier() {
    int pid = MyParentTid();
    IOReq req = { .type = NOTIFICATION };

    for (;;) {
        req.data = (unsigned int)AwaitEvent(UART1_RECV_EVENT);
        Send(pid, &req, sizeof(req), 0, 0);
    }
}

static void trainInServer() {
    char c = 0;
    int tid = 0;
    char taskb[TASK_MAX_TASKS];
    char charb[TRAIN_BUFFER_SIZE];
    CBuffer taskBuffer;
    CBuffer charBuffer;
    CBufferInit(&taskBuffer, taskb, TASK_MAX_TASKS);
    CBufferInit(&charBuffer, charb, TRAIN_BUFFER_SIZE);
    com1RecvSrvTid = MyTid();
    RegisterAs("trainInServer");

    IOReq req = { .type = -1, .data = '\0' };

    Create(PRIORITY_NOTIFIER, &trainInNotifier);

    for (;;) {
        Receive(&tid, &req, sizeof(req));

        switch (req.type) {
        // the notifier sent us a character
        case NOTIFICATION: {
            // unblock notifier
            Reply(tid, 0, 0);

            c = (char)req.data;
            if(CBufferIsEmpty(&taskBuffer)) {
                // no tasks waiting, put char in a buffer
                CBufferPush(&charBuffer, c);
            }
            else {
                // there are tasks waiting for a char
                // unblock a task and hand it the char
                // that we'd just acquired
                int popped_tid = (int)CBufferPop(&taskBuffer);

                Reply(popped_tid, &c, sizeof(c));
            }
            break;
        // some user task called getchar
        }
        case GETCHAR: {
            if(CBufferIsEmpty(&charBuffer)) {
                // no char to give back to caller; block it
                CBufferPush(&taskBuffer, (char)tid);
            }
            else {
                // characters in the buffer, return it
                c = CBufferPop(&charBuffer);
                Reply(tid, &c, sizeof(c));
            }
            break;
        }
        default:
            break;
        }
    }
}

static void trainOutNotifier() {
    int pid = MyParentTid();
    IOReq req = {
        .type = NOTIFICATION
    };

    *(int *)(UART1_BASE + UART_DATA_OFFSET) = 0x60;

    for (;;) {
        req.data = (unsigned int)AwaitEvent(UART1_XMIT_EVENT);
        Send(pid, &req, sizeof(req), 0, 0);
    }
}

static void trainOutServer() {
    int tid = 0;
    char * sendAddr = 0;
    char charb[TRAIN_BUFFER_SIZE];
    CBuffer charBuffer;
    CBufferInit(&charBuffer, charb, TRAIN_BUFFER_SIZE);
    com1SendSrvTid = MyTid();
    RegisterAs("trainOutServer");
    IOReq req = { .type = -1, .data = '\0' };

    // Spawn notifier
    int notifierTid = Create(PRIORITY_NOTIFIER, &trainOutNotifier);

    for (;;) {
        Receive(&tid, &req, sizeof(req));

        switch (req.type) {
        case NOTIFICATION: {
            if(CBufferIsEmpty(&charBuffer)) {
                // nothing to send
                // mark that we've seen a xmit interrupt
                // and block the notifier
                sendAddr = (char *)(req.data);
            }
            else {
                char ch = CBufferPop(&charBuffer);

                // write out char to volatile address
                // this also clears the xmit interrupt
                *((char *)req.data) = ch;

                // unblock the notifier
                Reply(tid, 0, 0);
            }
            break;
        }
        case PUTCHAR: {
            Reply(tid, 0, 0);

            // We've seen a xmit but did not
            // have a char to send at the moment
            if (sendAddr != 0)
            {
                // send the char directly
                // also clears the xmit interrupt
                *sendAddr = (char)(req.data);

                // unblock the notifier which will
                // call AwaitEvent() which re-enables
                // xmit interrupt
                Reply(notifierTid, 0, 0);

                // reset 'seen transmit int' state
                sendAddr = 0;
            }
            // Have not seen a xmit; just buffer that shit
            else
            {
                CBufferPush(&charBuffer, (char)(req.data));
            }
            break;
        }
        case PUTSTR: {
            // unblock caller
            Reply(tid, 0, 0);

            // get the ptr to str
            char *str = (char *)(req.data);

            // push entire string into buffer
            CBufferPushStr(&charBuffer, str);

            // unblock notifier we've previously
            // got a xmit but had nothing to send
            if (!CBufferIsEmpty(&charBuffer) &&
                sendAddr != 0)
            {
                *sendAddr = CBufferPop(&charBuffer);

                // unblock the notifier which will
                // call AwaitEvent() which re-enables
                // xmit interrupt
                Reply(notifierTid, 0, 0);

                // reset 'seen transmit int' state
                sendAddr = 0;
            }
        }
        default:
            break;
        }
    }
}

void initIOServers() {
    Create(PRIORITY_TRAIN_OUT_SERVER, trainOutServer);
    Create(PRIORITY_TRAIN_IN_SERVER, trainInServer);
    Create(PRIORITY_MONITOR_OUT_SERVER, monitorOutServer);
    Create(PRIORITY_MONITOR_IN_SERVER, monitorInServer);
}

void exitIOServers() {

}
