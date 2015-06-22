#include <utils.h>
#include <user/train.h>
#include <user/syscall.h>

#define REVERSE   15
#define STRAIGHT 33
#define CURVED 34
#define SOLENOID_OFF 32

void trainSetSpeed(int train_number, int train_speed) {
    Putc(COM1, train_speed);
    Putc(COM1, train_number);
}

void trainSetReverse(int train_number) {
    Putc(COM1, REVERSE);
    Putc(COM1, train_number);
}

void trainSetSwitch(int switch_number, bool curved) {
    Putc(COM1, curved ? CURVED : STRAIGHT);
    Putc(COM1, switch_number);
    Putc(COM1, SOLENOID_OFF);
}

