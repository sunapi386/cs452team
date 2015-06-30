#include <debug.h>
#include <utils.h>
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

// Create this with lowest priority of 31
// monitor performance statistics: task queue lengths, task CPU usage
inline void drawIdle(unsigned int diff) {
    String s;
    sinit(&s);
    sprintf(&s, "%s%s", VT_CURSOR_SAVE, VT_CURSOR_HIDE);
    vt_pos(&s, VT_IDLE_ROW, VT_IDLE_COL);
    sprintf(&s, "%u %% idle%s%s", diff, VT_CURSOR_SHOW, VT_CURSOR_RESTORE);
    PutString(COM2, &s);
}

void idleProfiler() {
    int i = 0;
    for (;;) {
        if(i++ % 500000 == 0) {
            drawIdle(taskIdleRatio());
        }
        Pass();
    }
}


void undefinedInstructionTesterTask() {
// Use online disassembler https://www.onlinedisassembler.com/odaweb/uSE7Vq1w/0
    asm volatile( "#0xffffffff\n\t" );
    debug("after");
}
