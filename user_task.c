#include <user_task.h>
#include <bwio.h>
#include <syscall.h>

static void childTask() {
    int tid = MyTid();
    int p_tid = MyParentTid();

    bwprintf(COM2, "Task: %d, Parent: %d\r\n", tid, p_tid);
    Pass();
    bwprintf(COM2, "Task: %d, Parent: %d\r\n", tid, p_tid);
    Exit();
}

// Creates two task of priority 0 and 2. First user task should have priority 1
void userModeTask() {
    for (int i = 0; i < 4; ++i) {
        bwprintf(COM2, "Created: %d\r\n", Create( (i < 2) ? 2 : 0, childTask ));
    }

    bwputstr(COM2, "First: exiting\r\n");
    Exit();
    bwputstr(COM2, "First: failed to exit!\r\n");
}
