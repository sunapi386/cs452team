#include <utils.h>
#include <priority.h>           // init
#include <user/vt100.h>         // constants
#include <user/syscall.h>       // Create

static void draw(int time) {
    String s;
    sinit(&s);
    vt_pos(&s, VT_CLOCK_ROW, VT_CLOCK_COL);
    sputc(&s, '0' + time / 3600000);
    sputc(&s, '0' + (time % 3600000) / 360000);
    sputc(&s, ':');
    sputc(&s, '0' + (time % 360000) / 60000);
    sputc(&s, '0' + (time % 60000) / 6000);
    sputc(&s, ':');
    sputc(&s, '0' + (time % 6000) / 1000);
    sputc(&s, '0' + (time % 1000) / 100);
    sputc(&s, '.');
    sputc(&s, '0' + (time % 100) / 10);
    PutString(COM2, &s);
}

void clockDrawer() {
    String s;
    sinit(&s);
    sputstr(&s, VT_CLEAR_SCREEN);
    PutString(COM2, &s);

    int t = Time();
    while(1) {
        draw(t);
        t += 10; // 10 ticks is 1 ms
        DelayUntil(t);
    }
}
