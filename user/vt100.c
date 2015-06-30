#include <utils.h>
#include <user/vt100.h>
#include <user/syscall.h>

void vt_pos(String *s, int row, int col) {
    sprintfstr(s, "%c[%d;%dH", ESC, row, col);
}
