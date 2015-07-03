#include <user/engineer.h>
#include <priority.h>
#include <debug.h> // assert
#include <user/syscall.h>
#include <utils.h> // printf
#include <user/train.h> // trainSetSpeed
#include <user/sensor.h> // sizeof(MessageToEngineer)

static int desired_speed = -1;
static int active_train = -1; // static for now, until controller is implemented

static void engineerTask() {
    RegisterAs("engineer");
    // Goto DESIRED_SPEED
    assert(active_train >= 0);
    trainSetSpeed(active_train, desired_speed);
    int num_sensor_readings = 0;


    { // scope this hack to jumpstart the engineer and unblock courier
        unsigned int buffer[16];
        int tid;
        for(;;) {
            Receive(&tid, &buffer, 16);
            assert(tid > 0);
            if(*buffer == 0xdeadbeef) {
                break; // throws away messages from courier
            }
        }
    }

    // ignore sensor readings, but still unblock couriers
    for(;;) {


        // set train speed
    }

}

static int engineerTaskId;

void initEngineer() {
    // Create(PRIORITY_TRACK_CONTROLLER_TASK, controllerSimulatorTask);
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
