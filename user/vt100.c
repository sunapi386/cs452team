#include <string.h>
#include <user/vt100.h>
#include <user/syscall.h>


void vt_init() {
    String s;
    sinit(&s);
    sprintf(&s, "%s", VT_CLEAR_SCREEN);
    sputstr(&s, VT_RESET);
    sputstr(&s, VT_CURSOR_HIDE);


    // set scrolling region
    sprintf(&s, "%c[%u;%ur", ESC, VT_PARSER_TOP, VT_PARSER_BOT);
    // ESC  [  Pt   ;  Pb   r
    // Selects top and bottom margins, defining the scrolling region.
    // Pt is line number of first line in the scrolling region.
    // Pb is line number of bottom line.
    // If Pt and Pb are not selected, the complete screen is used (no margins).
    sputstr(&s, "Loading...");
    vt_pos(&s, VT_PARSER_ROW, VT_PARSER_COL);
    sputstr(&s, VT_CURSOR_SHOW);
    PutString(&s);
}

void vt_pos(String *s, int row, int col) {
    sprintf(s, "%c[%d;%dH", ESC, row, col);
}
