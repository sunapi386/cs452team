#include <string.h>
#include <priority.h>           // init
#include <user/vt100.h>              // constants
#include <user/syscall.h>       // Create

static void draw(int time) {
    String s;
    sinit(&s);
    sputstr(&s, VT_CURSOR_SAVE);
    sputstr(&s, VT_CSI);
    sputuint(&s, VT_CLOCK_ROW, 10);
    sputc(&s, ';');
    sputuint(&s, VT_CLOCK_COL, 10);
    sputc(&s, 'H');
    sputc(&s, '0' + time);
    sputstr(&s, "ms");
    sputstr(&s, VT_CURSOR_RESTORE);

}

void clockDrawer() {
    int t = Time();
    while(1) {
        draw(t);
        t += 10; // 10 ticks is 1 ms
        DelayUntil(t);
    }
}


void initClockDrawer() {
    Create(PRIORITY_CLOCK_DRAWER, clockDrawer);
}
