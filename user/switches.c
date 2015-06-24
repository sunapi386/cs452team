#include <user/switches.h>
#include <user/vt100.h>
#include <user/syscall.h> // sending to screen
#include <utils.h> // printing to screen

#define SOLENOID_OFF 32
#define STRAIGHT 33
#define CURVED 34

void initSwitches() {
    struct String s;

    sinit(&s);
    vt_pos(&s, VT_SWITCH_ROW, 1);
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
        switchesSetCurved(i);
    }

    for (int i = 153; i <= 156; ++i) {
        switchesSetCurved(i);
    }
}

void switchesSetCurved(int location) {

}
void switchesSetStraight(int location) {

}
