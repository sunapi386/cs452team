#include <user/turnout.h>
#include <user/vt100.h>
#include <user/syscall.h> // sending to screen
#include <utils.h> // printf, bool, string
#include <debug.h> // assert

#define SOLENOID_OFF 32
#define STRAIGHT 33
#define CURVED 34

// track_bitmap is 32 bits, represents the state of the track switches
// valid switch_number are from 1 to 18, and 153 to 156 (inclusive)
// setting it to 1 means it is curved, 0 means straight
static int track_bitmap = 0;


void initSwitches() {
    struct String s;

    sinit(&s);
    vt_pos(&s, VT_SWITCH_ROW, VT_SWITCH_COL);
    sputstr(&s, "+--------+--------+--------+\r\n");
    sputstr(&s, "|   1: # |   2: # |   3: # |\r\n");
    sputstr(&s, "|   4: # |   5: # |   6: # |\r\n");
    sputstr(&s, "|   7: # |   8: # |   9: # |\r\n");
    sputstr(&s, "|  10: # |  11: # |  12: # |\r\n");
    PutString(COM2, &s);

    sinit(&s);
    sputstr(&s, "|  13: # |  14: # |  15: # |\r\n");
    sputstr(&s, "|  16: # |  17: # |  18: # |\r\n");
    sputstr(&s, "+--------+--------+--------+--------+\r\n");
    sputstr(&s, "| 153: # | 154: # | 155: # | 156: # |\r\n");
    sputstr(&s, "+--------+--------+--------+--------+");
    PutString(COM2, &s);

    for (int i = 1; i <= 18; ++i) {
        switchesSetStraight(i);
    }

    for (int i = 153; i <= 156; ++i) {
        switchesSetStraight(i);
    }
}

static inline int _to_bit_address(int switch_number) {
    int bit_address;
    if(switch_number >= 153 && switch_number <= 156) {
        bit_address = switch_number - 135; // range is 18 to 21 inclusive
    }
    else {
        assert(1 <= switch_number && switch_number <= 18);
        bit_address = switch_number - 1; // range is 0 to 17 inclusive
    }
    return bit_address;
}

static inline void _set_bitmap(int switch_number, char direction) {
    int bit_address = _to_bit_address(switch_number);
    int value = (direction == CURVED) ? 1 : 0;
    // https://stackoverflow.com/questions/47981
    // Changing the nth bit to x:
    // Setting the nth bit to either 1 or 0 can be achieved with the following:
    // number ^= (-x ^ number) & (1 << n);
    // Bit n will be set if x is 1, and cleared if x is 0.
    track_bitmap ^= (-value ^ track_bitmap) & (1 << bit_address);
}

static void _setSwitch(int switch_number, char curvature, char *color, char symbol) {
    // updates the screen for where the switch_number is
    _set_bitmap(switch_number, curvature);
    int bit_address = _to_bit_address(switch_number);
    int row = 0, col = 0;
    if(bit_address <= 17) {
        row = bit_address / 3 + 1;
        col = 9 * (bit_address % 3) + 8;
    }
    else {
        row = 8;
        col = 9 * (bit_address - 18) + 8;
    }

    // tell train controller to change direction
    printf(COM1, "%c%c%c", curvature, switch_number, SOLENOID_OFF);

    // draw on screen
    String s;
    sinit(&s);
    sputstr(&s, VT_CURSOR_SAVE);
    vt_pos(&s, VT_SWITCH_ROW + row, col);
    sprintf(&s, "%s%s%s%s", color, symbol, VT_RESET, VT_CURSOR_RESTORE);
    PutString(COM2, &s);
}

void switchesSetCurved(int switch_number) {
    _setSwitch(switch_number, CURVED, VT_CYAN, 'C');
}

void switchesSetStraight(int switch_number) {
    _setSwitch(switch_number, STRAIGHT, VT_GREEN, 'S');
}

bool switchesIsCurved(int switch_number) {
    int bit_address = _to_bit_address(switch_number);
    return (track_bitmap >> bit_address) & 0x1;
}
