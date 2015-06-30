#include <user/turnout.h>
#include <debug.h> // assert
#include <utils.h> // printf, bool, string
#include <priority.h>
#include <user/syscall.h> // sending to screen
#include <user/vt100.h>
#include <user/train.h>
#include <user/track_controller.h> // shared reply structure

#define SOLENOID_OFF 32
#define STRAIGHT 33
#define CURVED 34

// track_bitmap is 32 bits, represents the state of the track turnout
// valid turnout_number are from 1 to 18, and 153 to 156 (inclusive)
// setting it to 1 means it is curved, 0 means straight
static int track_bitmap = 0;

static inline int _to_bit_address(int turnout_number) {
    int bit_address;
    if(turnout_number >= 153 && turnout_number <= 156) {
        bit_address = turnout_number - 135; // range is 18 to 21 inclusive
    }
    else {
        assert(1 <= turnout_number && turnout_number <= 18);
        bit_address = turnout_number - 1; // range is 0 to 17 inclusive
    }
    return bit_address;
}

static inline void _set_bitmap(int turnout_number, char direction) {
    int bit_address = _to_bit_address(turnout_number);
    int value = (direction == CURVED) ? 1 : 0;
    // https://stackoverflow.com/questions/47981
    // Changing the nth bit to x:
    // Setting the nth bit to either 1 or 0 can be achieved with the following:
    // number ^= (-x ^ number) & (1 << n);
    // Bit n will be set if x is 1, and cleared if x is 0.
    track_bitmap ^= (-value ^ track_bitmap) & (1 << bit_address);
}

static void _setTurnout(int turnout_number, char curvature, char *color, char symbol) {
    // updates the screen for where the turnout_number is
    _set_bitmap(turnout_number, curvature);
    int bit_address = _to_bit_address(turnout_number);
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
    printf(COM1, "%c%c%c", curvature, turnout_number, SOLENOID_OFF);

    // draw on screen
    String s;
    sinit(&s);
    sputstr(&s, VT_CURSOR_SAVE);
    vt_pos(&s, VT_TURNOUT_ROW + row, col);
    sputstr(&s, color);
    sputc(&s, symbol);
    sputstr(&s, VT_RESET);
    sputstr(&s, VT_CURSOR_RESTORE);
    PutString(COM2, &s);
}

static void _updateTurnoutDisplay(int turnout_number, bool is_straight) {
    int bit_address = _to_bit_address(turnout_number);
    int row, col;
    switch(bit_address) {
        case  0: row = 1; col = 3; break;
        case  1: row = 2; col = 3; break;
        case  2: row = 3; col = 3; break;
        case  3: row = 4; col = 3; break;
        case  4: row = 5; col = 3; break;
        case  5: row = 6; col = 3; break;
        case  6: row = 1; col = 7; break;
        case  7: row = 2; col = 7; break;
        case  8: row = 3; col = 7; break;
        case  9: row = 4; col = 7; break;
        case 10: row = 5; col = 7; break;
        case 11: row = 6; col = 7; break;
        case 12: row = 1; col = 12; break;
        case 13: row = 2; col = 12; break;
        case 14: row = 3; col = 12; break;
        case 15: row = 4; col = 12; break;
        case 16: row = 5; col = 12; break;
        case 17: row = 6; col = 12; break;
        case 18: row = 1; col = 18; break;
        case 19: row = 2; col = 18; break;
        case 20: row = 3; col = 18; break;
        case 21: row = 4; col = 18; break;
        default: assert(0);
    }
    row += VT_TURNOUT_ROW;
    col += VT_TURNOUT_COL;

    String s;
    sinit(&s);
    sputstr(&s, VT_CURSOR_SAVE);
    sputstr(&s, VT_CURSOR_HIDE);
    vt_pos(&s, row, col);
    sputc(&s, is_straight ? 's' : 'c');
    sputstr(&s, VT_CURSOR_SHOW);
    sputstr(&s, VT_CURSOR_RESTORE);
    PutString(COM2, &s);
}

static int controller_id;
static void turnoutTask() {
    String s;
    sinit(&s);
    sprintf(&s, "%s%s", VT_CURSOR_SAVE, VT_CURSOR_HIDE);
    vt_pos(&s, VT_TURNOUT_ROW, VT_TURNOUT_COL);
    sputstr(&s, "--   TURNOUTES    --\r\n");
    sputstr(&s, "1:x  7:x 13:x 153:x\r\n");
    sputstr(&s, "2:x  8:x 14:x 154:x\r\n");
    sputstr(&s, "3:x  9:x 15:x 155:x\r\n");
    sputstr(&s, "4:x 10:x 16:x 156:x\r\n");
    sputstr(&s, "5:x 11:x 17:x\r\n");
    sputstr(&s, "6:x 12:x 18:x\r\n");
    sputstr(&s, VT_CURSOR_SHOW);
    sputstr(&s, VT_CURSOR_RESTORE);
    PutString(COM2, &s);

    for (int i = 1; i <= 18; ++i) {
        _setTurnout(i, CURVED, VT_CYAN, 'c');
    }

    for (int i = 153; i <= 156; ++i) {
        _setTurnout(i, CURVED, VT_CYAN, 'c');
    }

    controller_id = WhoIs("controller");
    assert(controller_id >= 0);
    ControllerData controller_reply;

    while(1) {
        Send(controller_id, 0, 0, &controller_reply, sizeof(controller_reply));
        int turnout_number = controller_reply.data.turnout.turnout_number;
        bool is_straight = controller_reply.data.turnout.direction == Straight;

        assert((153 <= turnout_number && turnout_number <= 156) ||
                 (1 <= turnout_number && turnout_number <= 18));
        assert(is_straight == true || is_straight == false);

        if(is_straight) {
            _setTurnout(turnout_number, STRAIGHT, VT_GREEN, 's');
        }
        else {
            _setTurnout(turnout_number, CURVED, VT_CYAN, 'c');
        }

    }

}

/** turnoutSet:
    a human calls this;
    controller just replies the turnoutTask,
*/
void turnoutSet(int turnout_number, bool desire_curved) {
    trainSetSwitch(turnout_number, desire_curved); // only use in turnout.c
    _updateTurnoutDisplay(turnout_number, ! desire_curved);
    // tell controller
    assert(controller_id >= 0);
    ControllerData update;
    // TODO: set the update fields so the controller understands this is updated
    Send(controller_id, &update, sizeof(update), 0, 0);
}


bool turnoutIsCurved(int turnout_number) {
    int bit_address = _to_bit_address(turnout_number);
    return (track_bitmap >> bit_address) & 0x1;
}

void initTurnout() {
    Create(PRIORITY_TURNOUT_TASK, turnoutTask);
}
