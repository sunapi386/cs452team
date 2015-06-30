#include <utils.h>
#include <user/vt100.h>
#include <user/syscall.h>

void vt_pos(String *s, int row, int col) {
    // sprintfstr(s, "%c[%d;%dH", ESC, row, col);
    sputstr(s, VT_CSI);
    sputint(s, row, 10);
    sputstr(s, VT_SEP);
    sputint(s, col, 10);
    sputstr(s, "H");
}
