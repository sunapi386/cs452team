#include <user/engineer.h>
#include <priority.h>
#include <debug.h> // assert
#include <user/syscall.h>
#include <utils.h> // printf
#include <user/train.h> // trainSetSpeed
#include <user/sensor.h> // sizeof(MessageToEngineer)
#include <user/nameserver.h>
#include <user/track_data.h>
#include <user/turnout.h> // turnoutIsCurved

#define ALPHA           0.85
#define NUM_SENSORS     (5 * 16)

static int desired_speed = -1;
static int active_train = -1; // static for now, until controller is implemented
static int pairs[NUM_SENSORS][NUM_SENSORS];
static track_node g_track[TRACK_MAX]; // This is guaranteed to be big enough.

// Courier: sensorServer -> engineer
static void engineerCourier() {
    int engineer = MyParentTid();
    int sensor = WhoIs("sensorServer");

    SensorRequest sensorReq;  // courier <-> sensorServer
    sensorReq.type = MESSAGE_ENGINEER_COURIER;
    SensorUpdate engineerReq; // courier <-> engineer

    for (;;) {
        Send(sensor, &sensorReq, sizeof(sensorReq), &engineerReq, sizeof(engineerReq));
        // printf("Send wrote a engineerReq: t %d s %d\n\r", engineerReq.time, engineerReq.sensor);
        Send(engineer, &engineerReq, sizeof(engineerReq), 0, 0);
    }
}

// function that looks up the distance between any two landmarks
// returns a distance in micrometre.
int umDistanceBetween(SensorUpdate from, SensorUpdate to) {
    if(from.sensor == to.sensor) return 0;
    // int group = sensor_update.sensor >> 8;
    // int offset = sensor_update.sensor & 0xff;
    // int index = 16 * group + offset - 1;

    int from_index = 16 * (from.sensor >> 8) + (from.sensor & 0xff) - 1;
    int to_index = 16 * (to.sensor >> 8) + (to.sensor & 0xff) - 1;
    int total_distance = 0;
    // printf(COM2, ">>>>>>>>>>>>>> from %d (sensor %s), to %d (sensor %s)\r\n",
        // from_index, g_track[from_index].name, to_index, g_track[to_index].name);

    int i;
    track_node *next_node = &g_track[from_index];
    for(i = 0; i < TRACK_MAX; i++) {
        // printf(COM2, "[%s].", next_node->name);
        if(next_node->type == NODE_BRANCH) {
            // char direction = (turnoutIsCurved(next_node->num) ? 'c' : 's');
            // printf(COM2, "%d(%c,v%d)\t", next_node->num, direction, turnoutIsCurved(next_node->num));
            total_distance += next_node->edge[turnoutIsCurved(next_node->num)].dist;
            next_node = next_node->edge[turnoutIsCurved(next_node->num)].dest;
        }
        else if(next_node->type == NODE_SENSOR ||
                next_node->type == NODE_MERGE) {
            // printf(COM2, ".%s\t", next_node->edge[DIR_AHEAD].dest->name);

            total_distance += next_node->edge[DIR_AHEAD].dist;
            next_node = next_node->edge[DIR_AHEAD].dest;
        }
        else {
            // printf(COM2, ".?\t");
            break;
        }

        if((next_node - &g_track[0]) == to_index) {
            // printf(COM2, "\r\ndistanceBetween: total_distance %d.\n\r", total_distance);
            break;
        }
    }
    if(i == TRACK_MAX) {
        printf(COM2, "umDistanceBetween: something went wrong: %d.\n\r", i);
    }
    return 1000 * total_distance;
}

// function that looks up the next landmark, using direction_is_forward
// returns a landmark
// Landmark getNextLandmark(Landmark current_landmark);


static bool direction_is_forward = true;
static void engineerTask() {
    Create(PRIORITY_ENGINEER_COURIER, engineerCourier);
    printf(COM2, "debug: engineerTask started >>>>>>>>>>>>> active_train %d desired_speed %d\r\n");
    trainSetSpeed(active_train, desired_speed);
    // at this point the train is up-to-speed
    int tid;
    SensorUpdate last_update = {0,0};
    SensorUpdate sensor_update = {0,0};
    for(;;) {
        int len = Receive(&tid, &sensor_update, sizeof(SensorUpdate));
        if(len != sizeof(SensorUpdate)) {
            printf(COM2, "Warning engineer Received garbage\r\n");
            continue;
        }

        // print out the direction of travel

        int group = sensor_update.sensor >> 8;
        int offset = sensor_update.sensor & 0xff;
        int index = 16 * group + offset - 1;
        int new_time = sensor_update.time;
        // printf(COM2, "engineer read %c%d (%d idx) %d ticks\r\n", group + 'A', offset, index, new_time);

        Reply(tid, 0, 0);

        // i. the real-time location of the train in form of landmark
        // for now define landmark as only sensor, then location is:

        int time_since_last_sensor = sensor_update.time - last_update.time;
        int um_dist_segment = umDistanceBetween(last_update, sensor_update);
        int current_velocity_in_um = um_dist_segment / time_since_last_sensor;
        int est_dist_since_last_sensor = (current_velocity_in_um * time_since_last_sensor);

        printf(COM2, "current_velocity %d um/tick, actual distance %d estimated distance %d\n\r",
            current_velocity_in_um,
            um_dist_segment,
            est_dist_since_last_sensor);
        //
        //
        // for amusement, display:
        // average_velocity = length_of_track_circle / time_between_same_sensor
        // current_deviation_from_avg_v = abs(current_velocity - average_velocity)

        // ii. lookup the next landmark and display the estimate ETA to it
        //

        // if(last_index >= 0) { // apply learning
        //     int last_time = pairs[index][last_index];
        //     int past_difference = pairs[index][last_index];
        //     int new_difference = new_time - last_time;
        //     pairs[index][last_index] =  new_difference * ALPHA +
        //                                 past_difference * (1 - ALPHA);
        // }
        // printf(COM2, "new: from %d to %d is %d\r\n",
        //     last_index, index, pairs[index][last_index]);

        // last_index = index;
        // printf(COM2, "debug: done loop\r\n");
        last_update.time = sensor_update.time;
        last_update.sensor = sensor_update.sensor;
    }

}

static int engineerTaskId = -1;

void initEngineer() {
    printf(COM2, "debug: initEngineer\r\n");
    for(int i = 0; i < NUM_SENSORS; i++) {
        for(int j = 0; j < NUM_SENSORS; j++) {
            pairs[i][j] = 0;
        }
    }
    direction_is_forward = true;
    engineerTaskId = Create(PRIORITY_ENGINEER, engineerTask);
    assert(engineerTaskId >= 0);
}

void engineerPleaseManThisTrain(int train_number, int speed) {
    active_train = train_number;
    desired_speed = speed;
}

void engineerParserGotReverseCommand() {
    direction_is_forward = ! direction_is_forward;
}

void engineerLoadTrackStructure(char which_track) {
    // draw track
    drawTrackLayoutGraph(which_track);
    if(which_track == 'a') {
        init_tracka(g_track);
    }
    else {
        init_trackb(g_track);
    }
}
