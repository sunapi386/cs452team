#include <string.h>
#include <user/vt100.h>
#include <user/syscall.h>


void vt_pos(String *s, int row, int col) {
    sprintf(s, "%c%d;%cH", VT_CSI, row, col);
}
