#include <utils.h>
#include <priority.h>           // init
#include <user/vt100.h>         // constants
#include <user/syscall.h>       // Create

static inline void draw(int time) {
    printf(COM2, "%s\033[1;68H%c%c:%c%c:%c%c.%c%s",
        VT_CURSOR_SAVE,
        '0' + time / 3600000,
        '0' + (time % 3600000) / 360000,
        '0' + (time % 360000) / 60000,
        '0' + (time % 60000) / 6000,
        '0' + (time % 6000) / 1000,
        '0' + (time % 1000) / 100,
        '0' + (time % 100) / 10,
        VT_CURSOR_RESTORE);
}

void clockDrawer() {
    int t = Time();
    while(1) {
        draw(t);
        t += 10; // 10 ticks is 1 ms
        DelayUntil(t);
    }
}
