// #include <user/switches.h>
// #include <string.h>
// #include <syscall.h> // sending to screen

// #define SOLENOID_OFF 32
// #define STRAIGHT 33
// #define CURVED 34

// void initSwitches() {
//     struct String s;
//     sinit(&s); vtPos(&s, VT_SWITCH_ROW, 1);
//     sinit(&s); sputstr(&s, "+--------+--------+--------+\r\n"); mioPrint(&s);
//     sinit(&s); sputstr(&s, "|   1: # |   2: # |   3: # |\r\n"); mioPrint(&s);
//     sinit(&s); sputstr(&s, "|   4: # |   5: # |   6: # |\r\n"); mioPrint(&s);
//     sinit(&s); sputstr(&s, "|   7: # |   8: # |   9: # |\r\n"); mioPrint(&s);
//     sinit(&s); sputstr(&s, "|  10: # |  11: # |  12: # |\r\n"); mioPrint(&s);
//     sinit(&s); sputstr(&s, "|  13: # |  14: # |  15: # |\r\n"); mioPrint(&s);
//     sinit(&s); sputstr(&s, "|  16: # |  17: # |  18: # |\r\n"); mioPrint(&s);
//     sinit(&s); sputstr(&s, "+--------+--------+--------+--------+\r\n"); mioPrint(&s);
//     sinit(&s); sputstr(&s, "| 153: # | 154: # | 155: # | 156: # |\r\n"); mioPrint(&s);
//     sputstr(&s, "+--------+--------+--------+--------+"); mioPrint(&s);

//     for (int i = 1; i <= 18; ++i) {
//         turnoutCurve(i,0);
//     }

//     for (int i = 153; i <= 156; ++i) {
//         turnoutCurve(i,0);
//     }
// }

// }
// void switchesSetCurved(int location) {

// }
// void switchesSetStraight(int location) {

// }
