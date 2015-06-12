#include <string.h>
#include <priority.h>           // init
#include <vt100.h>              // constants
#include <user/io.h>            // putStr
#include <user/clockserver.h>   // Time()

static inline cursor_move(String *s, unsigned row, unsigned col) {
    sputc(s, CSI);
    sputuint(s, row, 10);
    sputc(s, ";");
    sputuint(s, col, 10);
    sputc(s, "H");
}

static void draw(int time) {
    String s;
    sinit(&s);
    sputc(&s, CURSOR_SAVE);
    cursor_move(s, CLOCK_ROW, CLOCK_COL);
    sputc(&s, '0' + time);
    sputstr(&s, 'ms');
    sputc(&s, CURSOR_RESTORE);

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
