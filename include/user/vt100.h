#ifndef __VT100_H
#define __VT100_H

// http://www.vt100.net/docs/vt100-ug/chapter3.html

#define ESC 27

#define VT_ESC             "\033"      // Escape in octal
#define VT_CSI             VT_ESC "["     // Control Sequence Introducer
#define VT_SEP              ";"

#define VT_CURSOR_SAVE             VT_ESC "7"
#define VT_CURSOR_RESTORE          VT_ESC "8"
#define VT_CURSOR_SHOW             VT_ESC "?25h"
#define VT_CURSOR_HIDE             VT_ESC "?25l"

#define VT_CLEAR_LINE              VT_ESC "2K"
#define VT_CLEAR_SCREEN            VT_ESC "2J"


// COLORS
// http://www.termsys.demon.co.uk/vtansi.htm#colors
#define VT_COLOR_RESET                 VT_CSI "0m"
#define VT_COLOR_BLACK                 VT_CSI "30m"
#define VT_COLOR_RED                   VT_CSI "31m"
#define VT_COLOR_GREEN                 VT_CSI "32m"
#define VT_COLOR_YELLOW                VT_CSI "33m"
#define VT_COLOR_BLUE                  VT_CSI "34m"
#define VT_COLOR_MAGENTA               VT_CSI "35m"
#define VT_COLOR_CYAN                  VT_CSI "36m"
#define VT_COLOR_WHITE                 VT_CSI "37m"

#define VT_CARRIAGE_RETURN      0x0d

// Display positions
#define VT_CLOCK_ROW            1
#define VT_CLOCK_COL            30
#define VT_SENSOR_ROW           2
#define VT_SENSOR_COL           1

#define VT_PARSER_TOP           3
#define VT_PARSER_BOT           10
#define VT_PARSER_ROW           11
#define VT_PARSER_COL           0


void vt_init();
void vt_pos(struct String *s, int row, int col);

#endif
