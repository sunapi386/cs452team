#ifndef __VT100_H
#define __VT100_H

// http://www.vt100.net/docs/vt100-ug/chapter3.html


#define ESC             "\033"      // Escape
#define CSI             ESC "["     // Control Sequence Introducer

#define CURSOR_SAVE             ESC "7"
#define CURSOR_RESTORE          ESC "8"
#define CURSOR_SHOW             ESC "?25h"
#define CURSOR_HIDE             ESC "?25l"

#define CLEAR_LINE              ESC "2K"
#define CLEAR_SCREEN            ESC "2J"


// COLORS
// http://www.termsys.demon.co.uk/vtansi.htm#colors
#define COLOR_RESET                 CSI "0m"
#define COLOR_BLACK                 CSI "30m"
#define COLOR_RED                   CSI "31m"
#define COLOR_GREEN                 CSI "32m"
#define COLOR_YELLOW                CSI "33m"
#define COLOR_BLUE                  CSI "34m"
#define COLOR_MAGENTA               CSI "35m"
#define COLOR_CYAN                  CSI "36m"
#define COLOR_WHITE                 CSI "37m"

#define CLOCK_ROW            1
#define CLOCK_COL            30

#endif
