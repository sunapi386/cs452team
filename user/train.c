#include <user/train.h>
#include <string.h>
#include <user/io.h>

#define REVERSE   15
#define STRAIGHT 33
#define CURVED 34
#define SOLENOID_OFF 32

void trainSetSpeed(int train_number, int train_speed) {
    String s;
    sinit(&s);
    sprintf(&s, "%c%c", train_speed, train_number);
    PutString(&s);
}

void trainSetReverse(int train_number) {
    String s;
    sinit(&s);
    sprintf(&s, "%c%c", REVERSE, train_number);
}

void trainSetSwitch(int switch_number, bool curved) {
    String s;
    sinit(&s);
    sprintf(&s, "%c%c%c",
        curved ? CURVED : STRAIGHT,
        switch_number,
        SOLENOID_OFF);
}

