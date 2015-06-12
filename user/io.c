#include <user/io.h>
#include <user/syscall.h>
#include <events.h>
#include <utils.h>

#define NOTIFICATION 0
#define PUTCHAR      1
#define GETCHAR      2

typedef
struct IOReq {
    int type;
    char *data;
} IOReq;

static int com1RecvSrvTid = -1;
static int com2RecvSrvTid = -1;
static int com1SendSrvTid = -1;
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
    // puts the character into sendServer's buffer
    // Prepare request
    IOReq req = {
        .type = PUTCHAR,
        .data = (char *)c
    };
    int server_tid = (channel == COM1) ? com1SendSrvTid : com2SendSrvTid;
    int ret = Send(server_tid, &req, sizeof(req), 0, 0);
    return (ret < 0) ? -1 : 0;
}

static void receiveNotifier() {
    // messenger that waits for com1 or com2 to produce a character
    // and delivers to receiveServer
    int pid = MyParentTid();
    IOReq req = {
        .type = NOTIFICATION
    };

    for (;;) {
        // FIXME: change UART2_RECV_EVENT to also include UART1
        req.data = (char *)AwaitEvent(UART2_RECV_EVENT);
        Send(pid, &req, sizeof(req), 0, 0);
    }
}

// server that waits for receiveNotifier to deliver a character
void receiveServer() {
    char c = '\0';
    int tid = 0;
    char taskb[128];
    char charb[1024];
    CBuffer taskBuffer;
    CBuffer charBuffer;
    CBufferInit(&taskBuffer, (void*)taskb, 128);
    CBufferInit(&charBuffer, (void*)charb, 1024);

    IOReq req = {
        .type = -1,
        .data = '\0'
    };

    // Spawn notifier
    Create(1, &receiveNotifier);

    for (;;) {
        Receive(&tid, &req, sizeof(req));

        switch (req.type) {
        // the notifier sent us a character
        case NOTIFICATION:
        {
            // unblock notifier
            Reply(tid, 0, 0);

            // get volatile data
            c = *(req.data);

            if(CBufferIsEmpty(&taskBuffer)) {
                // no tasks waiting, put char in a buffer
                CBufferPush(&charBuffer, c);
            }
            else {
                // there are tasks waiting for a char
                // unblock a task and hand it the char
                // that we'd just acquired
                int popped_tid = CBufferPop(&taskBuffer);

                Reply(popped_tid, &c, sizeof(c));
            }
            break;
        }
        // some user task called getchar
        case GETCHAR:
            if(CBufferIsEmpty(&charBuffer)) {
                // no char to give back to caller; block it
                CBufferPush(&taskBuffer, tid);
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

static void sendNotifier() {
    int pid = MyParentTid();
    IOReq req = {
        .type = NOTIFICATION
    };

    for (;;) {
        req.data = (char *)AwaitEvent(UART2_XMIT_EVENT);
        Send(pid, &req, sizeof(req), 0, 0);
    }
}

// sendServer handles outbound traffic on COM ports
void sendServer() {
    int tid = 0;
    char * sendAddr = 0;
    char charb[1024];
    CBuffer charBuffer;
    CBufferInit(&charBuffer, (void*)charb, 1024);

    IOReq req = {
        .type = -1,
        .data = '\0'
    };

    // Spawn notifier
    Create(1, &sendNotifier);

    for (;;) {
        // Receive data
        Receive(&tid, &req, sizeof(req));

        switch (req.type) {
        case NOTIFICATION:
            if(CBufferIsEmpty(&charBuffer)) {
                // nothing to send
                // mark that we've seen a xmit interrupt
                sendAddr = (char *)(req.data);
            }
            else {
                // there is data to send
                Reply(tid, 0, 0);
                char ch = CBufferPop(&charBuffer);
                // write out char to volatile address
                *((char *)req.data) = ch;
            }
            break;
        case PUTCHAR:
            Reply(tid, 0, 0);
            // We've seen a xmit but did not
            // have a char to send at the moment
            if (sendAddr != 0)
            {
                // send the char directly
                *sendAddr = (char)(req.data);
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

