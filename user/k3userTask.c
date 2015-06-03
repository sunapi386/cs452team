#include <user/clockserver.h>
#include <user/nameserver.h>
#include <user/user_task.h>
#include <user/syscall.h>
#include <debug.h>

typedef struct {
    int duration;
    int times;
} DelayMsg;

static void clientTask() {
    debug("clientTasks");
    int start_time = Time();
    int tid = MyTid();
    int pid = MyParentTid();
    DelayMsg delay;

    // Get delay info from parent
    int ret = Send(pid, 0, 0, &delay, sizeof(DelayMsg));

    if(ret != sizeof(DelayMsg)) {
        debug("Expected clientTask ret %d got %d", sizeof(DelayMsg), ret);
        Exit();
    }

    int server = WhoIs("clockServer");
    debug("clockserver %d", server);

    // DelayMsg timing
    int i;
    for(i = 1; i <= delay.times; i++) {
        ret = Delay(delay.duration);
    }
    int end_time = Time();
    bwprintf(COM2, "clientTask %d interval %d delays completed %d total time %d, exiting..\n\r",
        tid, delay.duration, i - 1, end_time - start_time);

    Exit();
}

void userTaskK3() {
    int ret;
    int childs[4];
    int delay_times[4] = {20, 9, 6, 3};
    int delay_durations[4] = {10, 23, 33, 71};

    debug("creating nameserverTask");
    ret = Create(1, &nameserverTask);
    if(ret < 1) {
        debug("Expected create nameserverTask >= 1, got %d", ret);
        Exit();
    }
    debug("creating userTaskIdle");
    ret = Create(31, &userTaskIdle);
    if(ret < 1) {
        debug("Expected create userTaskIdle >= 1, got %d", ret);
        Exit();
    }
    // create clock server
    debug("creating clockServerTask");
    ret = Create(2, &clockServerTask);
    if(ret < 1) {
        debug("Expected create clockServerTask >= 1, got %d", ret);
        Exit();
    }

    // Create 4 client tasks
    for(int i = 0; i < 4; i++) {
        childs[i] = Create(3, &clientTask);
        debug("created clientTasks %d tid %d", i, childs[i]);

        if(childs[i] < 1) {
            debug("Expected Create to return id >= 1, got %d", childs[i]);
            Exit();
        }
    }

    // Execute receive 4 times
    for(int i = 0; i < 4; i ++) {
        int child_tid;
        ret = Receive(&child_tid, 0, 0);
        if(ret != 0) {
            debug("Expected Receive to return 0, got %d", ret);
            Exit();
        }
        DelayMsg delay;
        delay.times = delay_times[i];
        delay.duration = delay_durations[i];

        ret = Reply(child_tid, &delay, sizeof(DelayMsg));
        if(ret != 0) {
            debug("Expected Reply to return 0, got %d", ret);
        }
    }

    // Replies to each client in turn (blocks for children to end)
    debug("replying to childs");
    for(int i = 0; i < 4; i++) {
        Send(childs[i], 0, 0, 0, 0);
    }

    Exit();
}

// Create this with lowest priority of 31
void userTaskIdle() {
    debug("idling");
    for(int i = 0; i < 10000; i++);
    debug("exit");
    Exit();
}
