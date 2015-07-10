#include <user/clockserver.h>
#include <user/syscall.h>
#include <debug.h>
#include <events.h>
#include <priority.h>

#define MAX_DELAYED_TASKS 128
#define CLOCK_SERVER_NAME "clockServer"

typedef struct ClockReq {
    enum {
        NOTIFICATION,
        TIME,
        DELAY,
        DELAY_UNTIL,
        EXIT,
    } type;
    int data;
} ClockReq;

typedef struct DelayedTask {
    int tid;
    unsigned int finalTick;
    struct DelayedTask *next;
} DelayedTask;

typedef struct DelayedQueue {
    DelayedTask tasks[MAX_DELAYED_TASKS];
    DelayedTask *tail;
} DelayedQueue;

static int clockServerTid = -1;

int Time() {
    ClockReq req;
    req.type = TIME;
    Send(clockServerTid, &req, sizeof(req), &(req.data), sizeof(int));
    return req.data;
}

int Delay(int ticks) {
    ClockReq req;
    req.type = DELAY;
    req.data = ticks;
    Send(clockServerTid, &req, sizeof(ClockReq), 0, 0);
    return 0;
}

int DelayUntil(int ticks) {
    ClockReq req;
    req.type = DELAY_UNTIL;
    req.data = ticks;
    Send(clockServerTid, &req, sizeof(ClockReq), 0, 0);
    return 0;
}

static void initDelayedTasks(DelayedQueue *q) {
    for (int i = 0; i < MAX_DELAYED_TASKS; i++) {
        q->tasks[i].tid = i;
        q->tasks[i].finalTick = 0;
        q->tasks[i].next = 0;
    }
    q->tail = 0;
}

static void insertDelayedTask(DelayedQueue *q, int tid, unsigned int finalTick) {
    DelayedTask *task = &(q->tasks[tid]);
    task->finalTick = finalTick;

    if (q->tail == 0) {
        q->tail = task;
        task->next = task;
        return;
    }
    DelayedTask *curr = q->tail;
    for (;;) {
        if (curr->next->finalTick >= finalTick) {
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

static void removeExpiredTasks(DelayedQueue *q, unsigned int currTick) {
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

static void clockNotifier() {
    ClockReq req;
    req.type = NOTIFICATION;
    req.data = 0xdeadbeaf;
    for (;;) {
        AwaitEvent(TIMER_EVENT);
        Send(clockServerTid, &req, sizeof(req), 0, 0);
    }
}

static void clockServerTask() {
    clockServerTid = MyTid();
    int tid;
    unsigned tick = 0;
    ClockReq req;
    req.type = 0;
    req.data = 0;
    DelayedQueue q;
    initDelayedTasks(&q);
    RegisterAs("clockServer");
    int clockNotifierId = Create(PRIORITY_CLOCK_NOTIFIER, &clockNotifier);

    for (;;) {
        Receive(&tid, &req, sizeof(req));
        switch (req.type) {
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
        case EXIT:
            Reply(tid, 0, 0);
            goto cleanup;
        default:
            debug("[Clockserver] Invalid request type: %d\n\r", req.type);
            assert(0);
            break;
        }
    }
cleanup:
    Kill(clockNotifierId);
    Exit();
}

void initClockserver() {
    Create(PRIORITY_CLOCK_SERVER, clockServerTask);
}

void exitClockserver() {
    ClockReq request;
    request.type = EXIT;
    Send(clockServerTid, &request, sizeof(ClockReq), 0, 0);
}
