#include <user/clockserver.h>
#include <user/nameserver.h>
#include <user/syscall.h>
#include <kernel/timer.h>

#define NOTIFICATION 0
#define TIME         1
#define DELAY        2
#define DELAY_UNTIL  3

#define MAX_DELAYED_TASKS 128
#define TIMER_EVENT 51

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
    ClockReq req;

    for (;;)
    {
        AwaitEvent(TIMER_EVENT);
        Send(pid, &req, reqSize, 0, 0);
    }
}

static void initDelayedTasks(DelayedQueue *q,
                             DelayedTask tasks[])
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

static inline void insertDelayedTask(DelayedQueue *q,
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
    }
    else
    {
        DelayedTask *curr = q->tail;
        for (;;)
        {
            if (curr->next->finalTick >= finalTick)
            {
                // Insert node here
                task->next = curr->next;
                curr->next = task;
                break;
            }
            else if (curr->next == q->tail)
            {
                // Reached the end of the list;
                // insert node and update tail here
                task->next = curr->next;
                curr->next = task;
                q->tail = task;
                break;
            }
            curr = curr->next;
        }
    }
}

static inline void removeExpiredTasks(DelayedQueue *q,
                                      unsigned int currTick)
{
    if (q->tail == 0) return;
    DelayedTask *curr = q->tail->next;
    for (;;)
    {
        if (curr->finalTick > currTick)
        {
            // Tick is after current tick; means
            // that we are done. Set the head to
            // curr.
            q->tail->next = curr;
            break;
        }

        // Unblock task
        Reply(curr->tid, 0, 0);

        if (curr == q->tail)
        {
            // Queue is empty; set tail to NULL
            q->tail = 0;
            break;
        }

        curr = curr->next;
    }
}

void clockServerTask()
{
    // Initialize variables
    int tid = 0;
    unsigned int currTick = 0;
    DelayedTask tasks[MAX_DELAYED_TASKS];
    DelayedQueue q;
    ClockReq req;
    clockServerTid = MyTid();

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
            removeExpiredTasks(&q, currTick);
            break;
        }
        case TIME:
            // Reply with current tick
            Reply(tid, &currTick, sizeof(int));
            break;
        case DELAY:
        {
            // add to delayed queue
            insertDelayedTask(&q, tasks, tid, req.data + currTick);
            break;
        }
        case DELAY_UNTIL:
        {
            // add to delayed queue
            insertDelayedTask(&q, tasks, tid, req.data);
            break;
        }
        default:
            break;
        }
    }
}

