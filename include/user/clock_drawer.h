#include <string.h>
#include <priority.h>
#include <vt100.h>
#include <user/io.h>

static inline cursor_move(String *s, unsigned row, unsigned col) {
    sputc(s, CSI);
    sputuint(s, row, 10);
    sputc(s, ";");
    sputuint(s, col, 10);
    sputc(s, "H");
}

static void drawTime(int time) {
    String s;
    sinit(&s);
    sputc(&s, CURSOR_SAVE);
    cursor_move(s, CLOCK_ROW, CLOCK_COL);
    sputc(&s, '0' + time);
    sputstr(&s, 'ms');
    sputc(&s, CURSOR_RESTORE);

}

void clockDrawer() {
    //
}


void initClockDrawer() {
    Create(PRIORITY_CLOCK_DRAWER, clockDrawer);
}