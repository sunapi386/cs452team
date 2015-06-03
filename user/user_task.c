#include <bwio.h>
#include <user/syscall.h>
#include <user/user_task.h>

static void childTask() {
    int tid = MyTid();
    int p_tid = MyParentTid();

    char *msg = "hi";
    bwprintf(COM2, "Task: %d, Parent: %d Sending: %s.\r\n", tid, p_tid, msg);
    char reply[3];
    int len = Send(p_tid, msg, 3, reply, 3);
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
        bwprintf(COM2, "1st task received from %d message(%d): %s. ", tid, len, msg);
        char *reply = "IH";
        Reply(tid, reply, 3);
    }

    bwputstr(COM2, "First: exiting\r\n");
    Exit();
}
