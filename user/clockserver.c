#include <user/clockserver.h>
#include <user/syscall.h>
#include <debug.h>
#include <events.h>
#include <priority.h>

#define NOTIFICATION 0
#define TIME         1
#define DELAY        2
#define DELAY_UNTIL  3

#define MAX_DELAYED_TASKS 128
#define CLOCK_SERVER_NAME "clockServer"

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

int Time()
{
    static int clockServerTid = -1;
    if (clockServerTid < 0)
    {
        clockServerTid = WhoIs(CLOCK_SERVER_NAME);
    }
    ClockReq req;
    req.type = TIME;
    Send(clockServerTid, &req, sizeof(ClockReq), &(req.data), sizeof(int));
    return req.data;
}

int Delay(int ticks)
{
    static int clockServerTid = -1;
    if (clockServerTid < 0)
    {
        clockServerTid = WhoIs(CLOCK_SERVER_NAME);
    }
    ClockReq req;
    req.type = DELAY;
    req.data = ticks;
    Send(clockServerTid, &req, sizeof(ClockReq), 0, 0);
    return 0;
}

int DelayUntil(int ticks)
{
    static int clockServerTid = -1;
    if (clockServerTid < 0)
    {
        clockServerTid = WhoIs(CLOCK_SERVER_NAME);
    }
    ClockReq req;
    req.type = DELAY_UNTIL;
    req.data = ticks;
    Send(clockServerTid, &req, sizeof(ClockReq), 0, 0);
    return 0;
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
        tasks[i].finalTick = 0;
        tasks[i].next = 0;
    }
}

void insertDelayedTask(DelayedQueue *q,
                       DelayedTask tasks[],
                       int tid,
                       unsigned int finalTick)
{
    // Setup delayed task struct
    DelayedTask *task = &tasks[tid];
    task->finalTick = finalTick;

    if (q->tail == 0)
    {
        q->tail = task;
        task->next = task;
        return;
    }
    DelayedTask *curr = q->tail;
    for (;;)
    {
        if (curr->next->finalTick >= finalTick)
        {
            // Insert node here
            task->next = curr->next;
            curr->next = task;
            return;
        }

        if (curr->next == q->tail) {
            // Reached the end of the list;
            // insert node and update tail here
            task->next = curr->next;
            curr->next = task;
            q->tail = task;
            return;
        }
        curr = curr->next;
    }
}

void removeExpiredTasks(DelayedQueue *q, unsigned int currTick) {
    if (q->tail == 0) return;
    DelayedTask *curr = q->tail->next;
    for (;;) {
        if (curr->finalTick > currTick) {
            // Tick is after current tick; means
            // that we are done. Set the head to
            // curr.
            q->tail->next = curr;
            break;
        }
        // Unblock task
        Reply(curr->tid, 0, 0);

        if (curr == q->tail) {
            // Queue is empty; set tail to NULL
            q->tail = 0;
            break;
        }
        curr = curr->next;
    }
}

void clockNotifier()
{
    // Initialize variabels
    int pid = MyParentTid();
    ClockReq req;
    req.type = NOTIFICATION;

    for (;;)
    {
        AwaitEvent(TIMER_EVENT);
        Send(pid, &req, sizeof(ClockReq), 0, 0);
    }
}

void clockServerTask()
{
    // Initialize variables
    int tid = 0;
    unsigned int tick = 0;
    ClockReq req;
    req.type = 0;
    req.data = 0;
    DelayedQueue q;
    DelayedTask tasks[MAX_DELAYED_TASKS];
    initDelayedTasks(&q, tasks);

    // Register with name server
    RegisterAs("clockServer");

    // Spawn notifier
    Create(PRIORITY_CLOCK_NOTIFIER, &clockNotifier);

    // Main loop for serving requests
    for (;;)
    {
        Receive(&tid, &req, sizeof(ClockReq));
        switch (req.type)
        {
        case NOTIFICATION:
            Reply(tid, 0, 0);
            tick++;
            removeExpiredTasks(&q, tick);
            break;
        case TIME:
            Reply(tid, &tick, sizeof(tick));
            break;
        case DELAY:
            insertDelayedTask(&q, tasks, tid, req.data + tick);
            break;
        case DELAY_UNTIL:
            insertDelayedTask(&q, tasks, tid, req.data);
            break;
        default:
            break;
        }
    }
}
