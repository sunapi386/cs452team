#include <user/clockserver.h>
#include <user/nameserver.h>
#include <user/syscall.h>

#define NOTIFICATION 0
#define TIME         1
#define DELAY        2
#define DELAY_UNTIL  3

#define MAX_DELAYED_TASKS 128

typedef
struct ClockReq {
    int type;
    int data;
} ClockReq;

typedef
struct DelayedTask {
    int tid;
    unsigned int finalTick;
    struct DelayedTask *next;
} DelayedTask;

typedef
struct DelayedQueue {
    DelayedTask *tail;
} DelayedQueue;

static const int reqSize = sizeof(ClockReq);
static int clockServerTid = -1;

int Time()
{
    if (clockServerTid == -1)
    {
        return -1;
    }
    ClockReq req;
    req.type = TIME;
    Send(clockServerTid, &req, reqSize, &(req.data), sizeof(req.data));
    return req.data;
}

int Delay(int ticks)
{
    if (clockServerTid == -1)
    {
        return -1;
    }
    ClockReq req;
    req.type = DELAY;
    req.data = ticks;
    Send(clockServerTid, &req, reqSize, &(req.data), sizeof(req.data));
    return req.data;
}

int DelayUntil(int ticks)
{
    if (clockServerTid == -1)
    {
        return -1;
    }
    ClockReq req;
    req.type = DELAY_UNTIL;
    req.data = ticks;
    Send(clockServerTid, &req, reqSize, &(req.data), sizeof(req.data));
    return req.data;
}

static void clockNotifier()
{
    // Initialize variabels
    int pid = MyParentTid();

    // Initialize timer

    for (;;)
    {
        // AwaitEvent()
        // Send(pid,
    }
}

void initDelayedTasks(DelayedQueue *q, DelayedTask tasks[])
{
    // Initialize delayed queue
    q->tail = 0;

    // Initialize delayed task pool
    int i;
    for (i = 0; i < MAX_DELAYED_TASKS; i++)
    {
        tasks[i].tid = i;
        tasks[i].next = 0;
    }
}

void insertDelayedTask(DelayedQueue *q,
                       DelayedTask tasks[],
                       unsigned int finalTick)
{

}

void unblockDelayedTasks(DelayedQueue *q, int currTick)
{

}

void clockServer()
{
    // Initialize variables
    int tid = 0, i = 0;
    int delayCount = 0;
    unsigned int currTick = 0;
    DelayedTask tasks[MAX_DELAYED_TASKS];
    DelayedQueue q;
    ClockReq req;

    // Initialize data structures to store delayed tasks
    initDelayedTasks(&q, tasks);

    // Register with name server
    RegisterAs("clockServer");

    // Spawn notifier
    Create(1, &clockNotifier);

    // Main loop for serving requests
    for (;;)
    {
        Receive(&tid, &req, reqSize);
        switch (req.type)
        {
        case NOTIFICATION:
        {
            // Unblock notifier
            Reply(tid, 0, 0);

            // Increment tick
            ++currTick;

            // Unblock expired tasks
            unblockDelayedTasks(&q, currTick);
            break;
        }
        case TIME:
            // Reply with current tick
            Reply(tid, &currTick, sizeof(int));
            break;
        case DELAY:
        {
            // add to delayed queue
            insertDelayedTask(&q, tasks, req.data + currTick);
            break;
        }
        case DELAY_UNTIL:
        {
            // add to delayed queue
            insertDelayedTask(&q, tasks, req.data);
            break;
        }
        default:
            break;
        }
    }
}

