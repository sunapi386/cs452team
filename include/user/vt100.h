#ifndef __VT100_H
#define __VT100_H

// http://www.vt100.net/docs/vt100-ug/chapter3.html
// http://www.termsys.demon.co.uk/vtansi.htm#colors

#define ESC 27

#define VT_ESC             "\033"     // Escape in octal
#define VT_CSI             VT_ESC "["     // Control Sequence Introducer
#define VT_SEP              ";"

#define VT_CURSOR_SAVE             VT_ESC "7"
#define VT_CURSOR_RESTORE          VT_ESC "8"

#define VT_CLEAR_LINE              (VT_CSI "2K")
#define VT_CLEAR_SCREEN            (VT_CSI "2J")

#define VT_RESET                 (VT_CSI "0m")
#define VT_BLACK                 (VT_CSI "30m")
#define VT_RED                   (VT_CSI "31m")
#define VT_GREEN                 (VT_CSI "32m")
#define VT_YELLOW                (VT_CSI "33m")
#define VT_BLUE                  (VT_CSI "34m")
#define VT_MAGENTA               (VT_CSI "35m")
#define VT_CYAN                  (VT_CSI "36m")
#define VT_WHITE                 (VT_CSI "37m")

#define VT_CARRIAGE_RETURN      0x0d
#define VT_BACKSPACE            0x08

// Display positions
// refer to gui_track{A,B}.txt for layouts

#define VT_CLOCK_ROW            1
#define VT_CLOCK_COL            68
#define VT_IDLE_ROW             1
#define VT_IDLE_COL             1
#define VT_SENSOR_ROW           9
#define VT_SENSOR_COL           1
#define VT_TURNOUT_ROW          2
#define VT_TURNOUT_COL          1
#define VT_TRACK_GRAPH_ROW      4
#define VT_TRACK_GRAPH_COL      24
#define VT_ENGINEER_ROW         1
#define VT_ENGINEER_COL         80

#define VT_PARSER_TOP           18
#define VT_PARSER_BOT           20
#define VT_PARSER_ROW           21
#define VT_PARSER_COL           1

#define VT_LOG_ROW              26
#define VT_LOG_COL              1

struct String;
void vt_pos(struct String *s, int row, int col);

#endif
