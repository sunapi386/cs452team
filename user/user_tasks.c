#include <debug.h>
#include <utils.h>
#include <priority.h>
#include <kernel/task.h> // taskIdleRatio
#include <user/vt100.h>
#include <user/syscall.h>
#include <user/user_tasks.h>

static void childTask() {
    int tid = MyTid();
    int p_tid = MyParentTid();

    char *msg = "hi";
    bwprintf(COM2, "Task: %d, Parent: %d Sending: %s.\r\n", tid, p_tid, msg);
    char reply[3];
    int len = Send(p_tid, msg, 3, reply, 3);
    // int len = Send(p_tid, 0, 0, 0, 0);
    bwprintf(COM2, "Task: %d, Parent: %d Got reply(%d): %s.\r\n", tid, p_tid, len, reply);
    Exit();
}

// Creates two task of priority 0 and 2. First user task should have priority 1
void userTaskMessage() {
    bwprintf(COM2, "userModeTask tid: %d\r\n", MyTid());

    for (int i = 0; i < 4; ++i) {
        bwprintf(COM2, "Created: %d\r\n", Create( (i < 2) ? 2 : 0, childTask ));
    }

    for(int i = 0; i < 4; i++) {
        char msg[3];
        int tid = -1;
        int len = Receive(&tid, msg, 3);
        // int len = Receive(&tid, 0, 0);
        bwprintf(COM2, "1st task received from %d message(%d): %s. ", tid, len, msg);
        char *reply = "IH";
        Reply(tid, reply, 3);
        // Reply(tid, 0, 0);
    }

    bwputstr(COM2, "First: exiting\r\n");
    Exit();
}

#define BASE10  10
// Create this with lowest priority of 31
// monitor performance statistics: task queue lengths, task CPU usage
inline void drawIdle(unsigned int diff) {
    String s;
    sinit(&s);
    sputstr(&s, VT_CURSOR_SAVE);
    vt_pos(&s, VT_IDLE_ROW, VT_IDLE_COL);
    sputuint(&s, diff / 100, BASE10);
    sputc(&s, '.');
    sputuint(&s, diff % 100, BASE10);
    sputstr(&s, " %    ");
    sputstr(&s, VT_CURSOR_RESTORE);
    PutString(COM2, &s);
}

static void idleProfiler() {
    int i = 0;
    for (;;) {
        if(i++ % 2000000 == 0) {
            drawIdle(taskIdleRatio());
        }
    }
}

static int idleId;
void initIdleTask() {
    idleId = Create(PRIORITY_IDLE, idleProfiler);
}

void exitIdleTask() {
    Kill(idleId);
}

void undefinedInstructionTesterTask() {
// Use online disassembler https://www.onlinedisassembler.com/odaweb/uSE7Vq1w/0
    asm volatile( "#0xffffffff\n\t" );
    debug("after");
}

void clockWaiterTask(int task_to_notify) {
    int time = Time();
    while(1) {
        int len = Send(task_to_notify, &time, sizeof(int), &time, sizeof(int));
        assert(len == sizeof(int));
        if(time >= 0) {
            DelayUntil(time);
        }
        else {
            // should never get here
            return;
        }
    }
}
