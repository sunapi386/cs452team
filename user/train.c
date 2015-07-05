#include <utils.h>
#include <debug.h>
#include <user/train.h>
#include <user/syscall.h>

#define REVERSE   15
#define STRAIGHT 33
#define CURVED 34
#define SOLENOID_OFF 32
#define NUM_TRAINS 80

static unsigned short speeds[NUM_TRAINS];

void trainSetSpeed(int train_number, int train_speed) {
    assert(0 <= train_number && train_number <= 80);
    assert(0 <= train_speed && train_speed <= 14);
    Putc(COM1, train_speed);
    Putc(COM1, train_number);
    speeds[train_number] = train_speed;
}

void trainSetReverse(int train_number) {
    assert(0 <= train_number && train_number <= 80);
    Putc(COM1, REVERSE);
    Putc(COM1, train_number);
}

void trainSetReverseNicely(int train_number) {
    assert(0 <= train_number && train_number <= 80);
    unsigned short prev_speed = speeds[train_number];
    trainSetSpeed(train_number, 0);
    Delay(speeds[train_number] * 200);
    trainSetReverse(train_number);
    trainSetSpeed(train_number, prev_speed);
}

// DEPRECATED, only used by turnout.c
void trainSetSwitch(int switch_number, char direction) {
    assert((switch_number >= 153 && switch_number <= 156) ||
            (1 <= switch_number && switch_number <= 18));
    assert(direction == 'c' || direction == 's');
    char operation = (direction == 'c' || direction == 'C') ? CURVED : STRAIGHT;
    Putc(COM1, operation);
    Putc(COM1, switch_number);
    Putc(COM1, SOLENOID_OFF);
}

void initTrain() {
    for(int i = 0; i < NUM_TRAINS; i++) speeds[i] = 0;
}
