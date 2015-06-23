#include <utils.h>
#include <user/vt100.h>
#include <user/syscall.h>

void vt_init() {
    String s;
    sinit(&s);
    sputstr(&s, VT_CLEAR_SCREEN);
    sputstr(&s, VT_CURSOR_HIDE);
    sputstr(&s, VT_RESET);

    // set scrolling region
    sprintf(&s, "%s%d;%dr", VT_CSI, 18, 20);
    sputstr(&s, VT_CURSOR_SHOW);
    sputstr(&s, "Loading...");
    PutString(COM2, &s);
}

void vt_pos(String *s, int row, int col) {
    sprintf(s, "%c[%d;%dH", ESC, row, col);
}
