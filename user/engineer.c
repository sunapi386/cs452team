#include <user/engineer.h>
#include <priority.h>
#include <debug.h> // assert
#include <user/syscall.h>
#include <utils.h> // printf
#include <user/train.h> // trainSetSpeed
#include <user/sensor.h> // sizeof(MessageToEngineer)
#include <user/nameserver.h>

#define ALPHA           0.85
#define NUM_SENSORS     (5 * 16)

static int desired_speed = -1;
static int active_train = -1; // static for now, until controller is implemented
static int pairs[NUM_SENSORS][NUM_SENSORS];

// Courier: sensorServer -> engineer
static void engineerCourier() {
    int engineer = MyParentTid();
    int sensor = WhoIs("sensorServer");

    SensorRequest sensorReq;  // courier <-> sensorServer
    sensorReq.type = MESSAGE_ENGINEER_COURIER;
    SensorUpdate engineerReq; // courier <-> engineer

    for (;;) {
        Send(sensor, &sensorReq, sizeof(sensorReq), &engineerReq, sizeof(engineerReq));
        Send(engineer, &engineerReq, sizeof(engineerReq), 0, 0);
    }
}


static bool direction_is_forward = true;
static void engineerTask() {
    Create(PRIORITY_ENGINEER_COURIER, engineerCourier);

    // at this point the train is up-to-speed
    int tid;
    int last_index = -1;
    for(;;) {
        MessageToEngineer sensor_update;
        int len = Receive(&tid, &sensor_update, sizeof(MessageToEngineer));
        if(len != sizeof(MessageToEngineer)) {
            printf(COM2, "engineer Receive weird stuff\r\n");
            continue;
        }
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
            int past_difference = pairs[index][last_index];
            int new_difference = new_time - last_time;
            pairs[index][last_index] =  new_difference * ALPHA +
                                        past_difference * (1 - ALPHA);
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
    direction_is_forward = true;
}

void engineerPleaseManThisTrain(int train_number, int desired_speed) {
    active_train = train_number;
    desired_speed = desired_speed;
}

void engineerParserGotReverseCommand() {
    direction_is_forward = ! direction_is_forward;
}
