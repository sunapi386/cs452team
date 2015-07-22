#include <utils.h>
#include <debug.h>
#include <user/train.h>
#include <user/syscall.h>

#define REVERSE      15
#define STRAIGHT     33
#define CURVED       34
#define SOLENOID_OFF 32
#define NUM_TRAINS   80

static unsigned short speeds[NUM_TRAINS];

void trainSetSpeed(int train_number, int train_speed) {
    assert(1 <= train_number && train_number <= 80);
    assert(0 <= train_speed && train_speed <= 14);
    printf(COM1, "%c%c", (char)(train_speed), (char)(train_number));
    speeds[train_number] = train_speed;
}

void trainSetReverse(int train_number) {
    assert(1 <= train_number && train_number <= 80);
    printf(COM1, "%c%c", REVERSE, (char)(train_number));
}

void trainSetSwitch(int switch_number, char direction) {
    assert((switch_number >= 153 && switch_number <= 156) ||
            (1 <= switch_number && switch_number <= 18));
    assert(direction == 'c' || direction == 's' ||
           direction == 'C' || direction == 'S' );
    char operation = (direction == 'c' || direction == 'C') ? CURVED : STRAIGHT;
    printf(COM1, "%c%c%c", operation, switch_number, SOLENOID_OFF);
}

void initTrain() {
    for(int i = 0; i < NUM_TRAINS; i++) speeds[i] = 0;
}
