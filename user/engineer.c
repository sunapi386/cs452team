#include <user/engineer.h>
#include <priority.h>
#include <debug.h> // assert
#include <user/syscall.h>
#include <utils.h> // printf
#include <user/train.h> // trainSetSpeed
#include <user/sensor.h> // sizeof(MessageToEngineer)

#define ALPHA           0.85
#define NUM_SENSORS     (5 * 16)

static int desired_speed = -1;
static int active_train = -1; // static for now, until controller is implemented
static int pairs[NUM_SENSORS][NUM_SENSORS];

static void engineerTask() {
    // Goto DESIRED_SPEED
    assert(active_train >= 0);
    trainSetSpeed(active_train, desired_speed);

    { // to jumpstart the engineer and unblock courier
        unsigned int buffer[16];
        int tid;
        for(;;) {
            Receive(&tid, &buffer, 16);
            assert(tid > 0);
            if(*buffer == 0xdeadbeef) {
                break; // throws away messages from courier
            }
        }
        for(int num_updates = 0; num_updates < 5; num_updates++) {
            int _;
            Receive(&_, 0, 0);
        }
    }

    // at this point the train is up-to-speed
    int tid;
    int last_index = -1;
    for(;;) {
        MessageToEngineer sensor_update;
        Receive(&tid, &sensor_update, sizeof(MessageToEngineer));
        int group = sensor_update.sensor & 0xff00;
        int offset = sensor_update.sensor & 0xff;
        int index = 16 * group + offset - 1;
        int new_time = sensor_update.time;
        printf(COM2, "engineer read sensor: %c%d (%d index)at %d ticks\r\n",
            group + 'A',
            offset,
            index,
            new_time);

        if(last_index != -1) { // apply learning
            int last_time = pairs[index][last_index];
            pairs[index][last_index] = ALPHA * new_time + (1 - ALPHA) * last_time;
        }

        printf(COM2, "new: from %d to %d is %d\r\n",
            last_index, index, pairs[index][last_index]);

        last_index = index;
    }

}

static int engineerTaskId = -1;

void initEngineer() {
    for(int i = 0; i < NUM_SENSORS; i++) {
        for(int j = 0; j < NUM_SENSORS; j++) {
            pairs[i][j] = 0;
        }
    }
    engineerTaskId = Create(PRIORITY_ENGINEER, engineerTask);
    assert(engineerTaskId >= 0);
}

void engineerPleaseManThisTrain(int train_number, int desired_speed) {
    assert(engineerTaskId >= 0);
    int start_value = 0xdeadbeef;
    Send(engineerTaskId, &start_value, sizeof(int), 0, 0);
    active_train = train_number;
    desired_speed = desired_speed;
}
