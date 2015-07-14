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
    assert(1 <= train_number && train_number <= 80);
    assert(0 <= train_speed && train_speed <= 14);
    printf(COM2, "%c%c", (char)(train_speed), (char)(train_number));
    speeds[train_number] = train_speed;
}

void trainSetReverse(int train_number) {
    assert(1 <= train_number && train_number <= 80);
    printf(COM2, "%c%c", REVERSE, (char)(train_number));
}

void trainSetReverseNicely(int train_number) {
    assert(1 <= train_number && train_number <= 80);
    unsigned short prev_speed = speeds[train_number];
    trainSetSpeed(train_number, 0);
    Delay(300);
    trainSetReverse(train_number);
    Delay(15);
    trainSetSpeed(train_number, prev_speed);
}

// DEPRECATED, only used by turnout.c
void trainSetSwitch(int switch_number, char direction) {
    // debug("trainSetSwitch switch_number %d direction %d", switch_number, direction);
    assert((switch_number >= 153 && switch_number <= 156) ||
            (1 <= switch_number && switch_number <= 18));
    assert(direction == 'c' || direction == 's' ||
           direction == 'C' || direction == 'S' );
    char operation = (direction == 'c' || direction == 'C') ? CURVED : STRAIGHT;
    printf(COM1, "%c%c%c", operation, switch_number, SOLENOID_OFF);
}

void trainSetLight(int train_number, int on) {
    assert(1 <= train_number && train_number <= 80);
    if(on) {
        Putc(COM1, speeds[train_number] + 16);
        Putc(COM1, train_number);
    }
    else {
        Putc(COM1, speeds[train_number]);
        Putc(COM1, train_number);
    }
}

void initTrain() {
    for(int i = 0; i < NUM_TRAINS; i++) speeds[i] = 0;
}
