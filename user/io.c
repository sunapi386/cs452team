#include <user/io.h>
#include <user/syscall.h>
#include <events.h>
#include <utils.h>
#include <priority.h>
#include <string.h>
#include <ts7200.h>

#define NOTIFICATION 0
#define PUTCHAR      1
#define PUTSTR       2
#define GETCHAR      3
#define ECHO         4

typedef
struct IOReq {
    int type;
    unsigned int data;
} IOReq;

// TODO: Remove and replace with nameserver lookups
static int com1RecvSrvTid = -1;
static int com1SendSrvTid = -1;
static int com2RecvSrvTid = -1;
static int com2SendSrvTid = -1;

int Getc(int channel) {
    // Check channel type
    if (channel != COM1 && channel != COM2) return -1;

    // Prepare request
    IOReq req = {
        .type = GETCHAR
    };
    int server_tid = (channel == COM1) ? com1RecvSrvTid : com2RecvSrvTid;
    char c = 0;
    int ret = Send(server_tid, &req, sizeof(req), &c, sizeof(c));
    return ret < 0 ? ret : c;
}

int Putc(int channel, char c) {
    // Check channel type
    if (channel != COM1 && channel != COM2) return -1;

    // Prepare request
    IOReq req = {
        .type = PUTCHAR,
        .data = (unsigned int)c
    };
    int server_tid = (channel == COM1) ? com1SendSrvTid : com2SendSrvTid;
    int ret = Send(server_tid, &req, sizeof(req), 0, 0);
    return (ret < 0) ? -1 : 0;
}

int PutString(String *s) {
    return PutStr(s->buf);
}

int PutStr(char *str)
{
    // Prepare request
    IOReq req = {
        .type = PUTSTR,
        .data = (unsigned int)str
    };

    int ret = Send(com2SendSrvTid, &req, sizeof(req), 0, 0);
    return (ret < 0) ? -1 : 0;
}

static void monitorInNotifier() {
    int pid = MyParentTid();
    IOReq req = {
        .type = NOTIFICATION
    };

    for (;;) {
        req.data = (unsigned int)AwaitEvent(UART2_RECV_EVENT);
        Send(pid, &req, sizeof(req), 0, 0);
    }
}

static void echoCourier() {
    IOReq echoReq = {
        .type = ECHO
    };
    IOReq sendReq = {
        .type = PUTCHAR
    };

    for (;;) {
        // Request for a char from receiveServer()
        Send(com2RecvSrvTid, &echoReq, sizeof(echoReq), &(sendReq.data), sizeof(sendReq.data));

        // Send the char to sendServer() using a Putc request
        Send(com2SendSrvTid, &sendReq, sizeof(sendReq), 0, 0);
    }
}

void monitorInServer() {
    char c = 0;
    int tid = 0, courierTid = 0;
    char taskb[128];
    char charb[1024];
    CBuffer taskBuffer;
    CBuffer charBuffer;
    CBufferInit(&taskBuffer, taskb, 128);
    CBufferInit(&charBuffer, charb, 1024);

    IOReq req = {
        .type = -1,
        .data = '\0'
    };

    // TODO: nameserver
    com2RecvSrvTid = MyTid();

    RegisterAs("monitorInServer");

    // Spawn courier
    Create(PRIORITY_NOTIFIER, &echoCourier);

    // Spawn notifier
    Create(PRIORITY_NOTIFIER, &monitorInNotifier);

    for (;;) {
        Receive(&tid, &req, sizeof(req));

        switch (req.type) {
        // the notifier sent us a character
        case NOTIFICATION:
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
        // some user task called getchar
        case GETCHAR:
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
        // echo wants a character, block it until the next notification
        case ECHO:
            // block the echoer
            courierTid = tid;
            break;
        default:
            break;
        }
    }
}

static void monitorOutNotifier() {
    int pid = MyParentTid();
    IOReq req = {
        .type = NOTIFICATION
    };

    for (;;) {
        req.data = (unsigned int)AwaitEvent(UART2_XMIT_EVENT);
        Send(pid, &req, sizeof(req), 0, 0);
    }
}

void monitorOutServer() {
    int tid = 0;
    char * sendAddr = 0;
    char charb[1024];
    CBuffer charBuffer;
    CBufferInit(&charBuffer, charb, 1024);

    IOReq req = {
        .type = -1,
        .data = '\0'
    };

    // TODO: nameserver
    com2SendSrvTid = MyTid();

    // Register with name server
    RegisterAs("monitorOutServer");

    // Spawn notifier
    int notifierTid = Create(PRIORITY_NOTIFIER, &monitorOutNotifier);

    for (;;) {
        Receive(&tid, &req, sizeof(req));

        switch (req.type) {
        case NOTIFICATION:
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
        case PUTCHAR:
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
        case PUTSTR:
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
        default:
            break;
        }
    }
}

static void trainInNotifier() {
    int pid = MyParentTid();
    IOReq req = {
        .type = NOTIFICATION
    };

    for (;;) {
        req.data = (unsigned int)AwaitEvent(UART1_RECV_EVENT);
        Send(pid, &req, sizeof(req), 0, 0);
    }
}

void trainInServer() {
    char c = 0;
    int tid = 0;
    char taskb[128];
    char charb[1024];
    CBuffer taskBuffer;
    CBuffer charBuffer;
    CBufferInit(&taskBuffer, taskb, 128);
    CBufferInit(&charBuffer, charb, 1024);

    IOReq req = {
        .type = -1,
        .data = '\0'
    };

    // TODO: nameserver
    com1RecvSrvTid = MyTid();

    RegisterAs("trainInServer");

    // Spawn notifier
    Create(PRIORITY_NOTIFIER, &trainInNotifier);

    for (;;) {
        Receive(&tid, &req, sizeof(req));

        switch (req.type) {
        // the notifier sent us a character
        case NOTIFICATION:
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
        case GETCHAR:
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

void trainOutServer() {
    int tid = 0;
    char * sendAddr = 0;
    char charb[1024];
    CBuffer charBuffer;
    CBufferInit(&charBuffer, charb, 1024);

    IOReq req = {
        .type = -1,
        .data = '\0'
    };

    // TODO: Remove
    com1SendSrvTid = MyTid();

    // Register with name server
    RegisterAs("trainOutServer");

    // Spawn notifier
    int notifierTid = Create(PRIORITY_NOTIFIER, &trainOutNotifier);

    for (;;) {
        Receive(&tid, &req, sizeof(req));

        switch (req.type) {
        case NOTIFICATION:
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
        case PUTCHAR:
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
        default:
            break;
        }
    }
}
