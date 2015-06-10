#include <user/io.h>
#include <user/syscall.h>
#include <events.h>

#define NOTIFICATION 0
#define PUTCHAR      1
#define GETCHAR      2

typedef
struct IOReq {
    int type;
    char data;
} IOReq;

static int com1RecvSrvTid = -1;
static int com2RecvSrvTid = -1;

int Getc(int channel)
{
    // Check channel type
    if (channel != COM1 && channel != COM2) return -1;

    // Prepare request
    IOReq req = {
        .type = GETCHAR
    };

    int ret = Send(channel == COM1 ? com1RecvSrvTid : com2RecvSrvTid,
                    &req, sizeof(req), &(req.data), sizeof(req.data));
    return ret < 0 ? ret : req.data;
}

int Putc(int channel, char c)
{

}

void receiveNotifier()
{
    int pid = MyParentTid();
    IOReq req;
    req.type = NOTIFICATION;

    for (;;)
    {
        req.data = AwaitEvent(UART2_RECV_EVENT);
        Send(pid, &req, sizeof(IOReq), 0, 0);
    }
}

void receiveServer()
{
    // Initialize variables
    int tid = 0;
    // TODO: task queue
    // TODO: char buffer

    IOReq req = {
        .type = 0,
        .data = '\0'
    };

    // Spawn notifier
    Create(1, &receiveNotifier);

    for (;;)
    {
        Receive(&tid, &req, sizeof(IOReq));
        switch (req.type)
        {
        case NOTIFICATION:
            Reply(tid, 0, 0);
            // if there are tasks waiting on Getc, unblock a task and hand it a char
            // if there are no tasks waiting, put char in a buffer
            break;
        case GETCHAR:
            // if there are characters in the buffer, return it
            // else, queue task
            break;
        default:
            break;
        }
    }
}