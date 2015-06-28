#include <user/clockserver.h>
#include <user/syscall.h>
#include <debug.h>
#include <events.h>
#include <priority.h>

#define NOTIFICATION 5
#define TIME         6
#define DELAY        7
#define DELAY_UNTIL  8

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
    DelayedTask tasks[MAX_DELAYED_TASKS];
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
    Send(clockServerTid, &req, sizeof(req), &(req.data), sizeof(int));
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

void initDelayedTasks(DelayedQueue *q)
{
    // Initialize delayed task pool
    int i;
    for (i = 0; i < MAX_DELAYED_TASKS; i++)
    {
        q->tasks[i].tid = i;
        q->tasks[i].finalTick = 0;
        q->tasks[i].next = 0;
    }

    // Initialize delayed queue
    q->tail = 0;
}

void insertDelayedTask(DelayedQueue *q,
                       int tid,
                       unsigned int finalTick)
{
    // Setup delayed task struct
    DelayedTask *task = &(q->tasks[tid]);
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
    req.data = 0xdeadbeaf;
    for (;;)
    {
        AwaitEvent(TIMER_EVENT);
        Send(pid, &req, sizeof(req), 0, 0);
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
    initDelayedTasks(&q);

    // Register with name server
    RegisterAs("clockServer");

    // Spawn notifier
    Create(PRIORITY_CLOCK_NOTIFIER, &clockNotifier);

    // Main loop for serving requests
    for (;;)
    {
        Receive(&tid, &req, sizeof(req));

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
            insertDelayedTask(&q, tid, req.data + tick);
            break;
        case DELAY_UNTIL:
            insertDelayedTask(&q, tid, req.data);
            break;
        default:
            bwprintf(COM2, "[Clockserver] Invalid request type: %d\n\r", req.type);
            for (;;);
            break;
        }
    }
}
