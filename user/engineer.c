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

// function that looks up the distance between any two landmarks
// returns a distance in mm.
int distanceBetween(Landmark a, Landmark b);

// function that looks up the next landmark, using direction_is_forward
// returns a landmark
Landmark getNextLandmark(Landmark current_landmark);


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

        // print out the direction of travel

        int group = sensor_update.sensor & 0xff00;
        int offset = sensor_update.sensor & 0xff;
        int index = 16 * group + offset - 1;
        int new_time = sensor_update.time;
        printf(COM2, "engineer read sensor: %c%d (%d index)at %d ticks\r\n",
            group + 'A',
            offset,
            index,
            new_time);

        // i. the real-time location of the train in form of landmark
        // for now define landmark as only sensor, then location is:
        // estimated_distance_travelled_from_last_sensor =
        // (current_velocity * time_since_last_sensor)
        //
        // time_since_last_sensor = Time() - time_last_sensor_triggered
        //
        // current_velocity = distance_between_previous_two_sensors / time_between_last_sensor
        // for amusement, display:
        // average_velocity = length_of_track_circle / time_between_same_sensor
        // current_deviation_from_avg_v = abs(current_velocity - average_velocity)

        // ii. lookup the next landmark and display the estimate ETA to it
        //

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
    direction_is_forward = true;
    engineerTaskId = Create(PRIORITY_ENGINEER, engineerTask);
    assert(engineerTaskId >= 0);
}

void engineerPleaseManThisTrain(int train_number, int desired_speed) {
    active_train = train_number;
    desired_speed = desired_speed;
}

void engineerParserGotReverseCommand() {
    direction_is_forward = ! direction_is_forward;
}
