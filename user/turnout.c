#include <user/turnout.h>
#include <debug.h> // assert
#include <utils.h> // printf, bool, string
#include <priority.h>
#include <user/syscall.h> // sending to screen
#include <user/vt100.h>
#include <user/train.h>
#include <user/trackserver.h> // shared reply structure

#define SOLENOID_OFF 32
#define STRAIGHT 33
#define CURVED 34

// track_bitmap is 32 bits, represents the state of the track turnout
// valid turnout_number are from 1 to 18, and 153 to 156 (inclusive)
// setting it to 1 means it is curved, 0 means straight
static int track_bitmap = 0;

static int _to_bit_address(int turnout_number) {
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

static void _set_bitmap(int turnout_number, char direction) {
    assert( (turnout_number >= 153 && turnout_number <= 156) ||
            (1 <= turnout_number && turnout_number <= 18));
    assert(direction == 'c' || direction == 's');
    int bit_address = _to_bit_address(turnout_number);
    // https://stackoverflow.com/questions/47981
    if(direction == 'c') {
        // set bit number |= 1 << x;
        track_bitmap |= (1 << bit_address);
    }
    else {
        // clear bit number &= ~(1 << x);
        track_bitmap &= ~(1 << bit_address);
    }
}

static void _updateTurnoutDisplay(int turnout_number, char direction) {
    int bit_address = _to_bit_address(turnout_number);
    int row, col;
    switch(bit_address) {
        case  0: row = 1; col = 2; break;
        case  1: row = 2; col = 2; break;
        case  2: row = 3; col = 2; break;
        case  3: row = 4; col = 2; break;
        case  4: row = 5; col = 2; break;
        case  5: row = 6; col = 2; break;
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
        default: debug("error bit_address %d", bit_address); assert(0);
    }
    row += VT_TURNOUT_ROW;
    col += VT_TURNOUT_COL;
    assert(direction == 'c' || direction == 's');

    String s;
    sinit(&s);
    sputstr(&s, VT_CURSOR_SAVE);
    vt_pos(&s, row, col);
    sputstr(&s, (direction == 'c' ? VT_CYAN : VT_GREEN));
    sputc(&s, (direction == 'c' ? 'C' : 'S'));
    sputstr(&s, VT_RESET);
    sputstr(&s, VT_CURSOR_RESTORE);
    PutString(COM2, &s);
}

void setTurnout(int turnout_number, char direction) {
    // updates the screen for where the turnout_number is
    _set_bitmap(turnout_number, direction);
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
    trainSetSwitch(turnout_number, direction); // only use in turnout.c

    // draw on screen
    _updateTurnoutDisplay(turnout_number, direction);
}

void printResetTurnouts() {
    String s;
    sinit(&s);
    sputstr(&s, VT_CURSOR_SAVE);
    vt_pos(&s, VT_TURNOUT_ROW, VT_TURNOUT_COL);
    sputstr(&s, "--   TURNOUTES    --\r\n");
    sputstr(&s, "1:x  7:x 13:x 153:x\r\n");
    sputstr(&s, "2:x  8:x 14:x 154:x\r\n");
    sputstr(&s, "3:x  9:x 15:x 155:x\r\n");
    sputstr(&s, "4:x 10:x 16:x 156:x\r\n");
    sputstr(&s, "5:x 11:x 17:x\r\n");
    sputstr(&s, "6:x 12:x 18:x\r\n");
    sputstr(&s, VT_CURSOR_RESTORE);
    PutString(COM2, &s);

    for (int i = 1; i <= 18; ++i) {
        if (i == 11) {
            setTurnout(i, 'c');
            continue;
        }
        setTurnout(i, 's');
    }

    for (int i = 153; i <= 156; ++i) {
        setTurnout(i, 's');
    }
}

// static int trackServerId;
// static void turnoutTask() {
    /* Commenting out turnout task because trackserver no longer supports this.
    In the future the engineer would be communicating here and switching
    his own turnouts.
    printResetTurnouts();
    trackServerId = WhoIs("trackServer");
    assert(trackServerId >= 0);
    TrackRequest trackServerReply;

    while(1) {
        Send(trackServerId, 0, 0, &trackServerReply, sizeof(trackServerReply));
        int turnout_number = trackServerReply.data.turnout.turnout_number;
        char direction = trackServerReply.data.turnout.direction;

        assert((153 <= turnout_number && turnout_number <= 156) ||
                 (1 <= turnout_number && turnout_number <= 18));
        assert(direction == 'c' || direction == 's');

        setTurnout(turnout_number, direction);
    }
    */
// }

bool turnoutIsCurved(int turnout_number) {
    // checking a bit: bit = (number >> x) & 1;
    int bit_address = _to_bit_address(turnout_number);
    return (track_bitmap >> bit_address) & 0x1;
}

void initTurnout() {
    // Create(PRIORITY_TURNOUT_TASK, turnoutTask);
}
