#include <string.h>
#include <priority.h>           // init
#include <user/vt100.h>              // constants
#include <user/syscall.h>       // Create

static void draw(int time) {
    String s;
    sinit(&s);
    vt_pos(&s, VT_CLOCK_ROW, VT_CLOCK_COL);
    sprintf(&s, "%d ", time);
    PutString(&s);
}

void clockDrawer() {
    int t = Time();
    while(1) {
        draw(t);
        t += 10; // 10 ticks is 1 ms
        DelayUntil(t);
    }
}
