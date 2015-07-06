#include <user/engineer.h>
#include <priority.h>
#include <debug.h> // assert
#include <user/syscall.h>
#include <utils.h> // printf
#include <user/train.h> // trainSetSpeed
#include <user/sensor.h> // sizeof(MessageToEngineer)
#include <user/track_data.h>
#include <user/turnout.h> // turnoutIsCurved
#include <user/vt100.h>

#define ALPHA           85
#define NUM_SENSORS     (5 * 16)
#define NUM_TRAINS      80

static inline int abs(int num) {
    return (num < 0 ? -1 * num : num);
}

static int desired_speed = -1;
static int active_train = -1; // static for now, until controller is implemented
static int pairs[NUM_SENSORS][NUM_SENSORS];
static track_node g_track[TRACK_MAX]; // This is guaranteed to be big enough.
// static bool activated_engineers[NUM_TRAINS];

// Courier: sensorServer -> engineer
static void engineerCourier() {
    int engineer = MyParentTid();
    int sensor = WhoIs("sensorServer");

    SensorRequest sensorReq;  // courier <-> sensorServer
    sensorReq.type = MESSAGE_ENGINEER_COURIER;
    SensorUpdate result;  // courier <-> sensorServer
    MessageToEngineer engineerReq; // courier <-> engineer
    engineerReq.type = update_sensor;

    for (;;) {
        Send(sensor, &sensorReq, sizeof(sensorReq), &result, sizeof(result));
        engineerReq.data.update_sensor.sensor = result.sensor;
        engineerReq.data.update_sensor.time = result.time;
        Send(engineer, &engineerReq, sizeof(engineerReq), 0, 0);
    }
}

static inline int indexFromSensorUpdate(SensorUpdate update) {
    return 16 * (update.sensor >> 8) + (update.sensor & 0xff) - 1;
}

// function that looks up the distance between any two landmarks
// returns a distance in micrometre.
int umDistanceBetween(SensorUpdate from, SensorUpdate to) {
    if(from.sensor == to.sensor) return 0;
    // int group = sensor_update.sensor >> 8;
    // int offset = sensor_update.sensor & 0xff;
    // int index = 16 * group + offset - 1;

    int from_index = indexFromSensorUpdate(from);
    int to_index = indexFromSensorUpdate(to);
    int total_distance = 0;
    // printf(COM2, ">>>>>>>>>>>>>> from %d (sensor %s), to %d (sensor %s)\r\n",
        // from_index, g_track[from_index].name, to_index, g_track[to_index].name);

    int i;
    track_node *next_node = &g_track[from_index];
    // TODO: track_node *next_node = getNextLandmark(&g_track[from_index]);
    for(i = 0; i < TRACK_MAX; i++) {
        // printf(COM2, "[%s].", next_node->name);
        if(next_node->type == NODE_BRANCH) {
            // char direction = (turnoutIsCurved(next_node->num) ? 'c' : 's');
            // printf(COM2, "%d(%c,v%d)\t",
            //      next_node->num, direction, turnoutIsCurved(next_node->num));
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
        // TODO: next_node = getNextLandmark(next_node);
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

track_edge *getNextEdge(track_node *current_landmark) {
    if(current_landmark->type == NODE_BRANCH) {
        return &current_landmark->edge[turnoutIsCurved(current_landmark->num)];
    }
    else if(current_landmark->type == NODE_SENSOR ||
            current_landmark->type == NODE_MERGE) {
        return &current_landmark->edge[DIR_AHEAD];
    }
    else {
        printf(COM2, "getNextEdge: something went wrong: %c.\n\r", current_landmark->name);
    }
    return 0;

}

// function that looks up the next landmark, using direction_is_forward
// returns a landmark
track_node *getNextLandmark(track_node *current_landmark) {
    track_edge *next_edge = getNextEdge(current_landmark);
    return next_edge->dest;
}

static bool direction_is_forward = true;
static void updateScreen(char group, int offset, int expected_time,
                        int actual_time, int error) {
    // draws the info nicely so we don't flood the screen with updates
    String s;
    sinit(&s);
    sputstr(&s, VT_CURSOR_SAVE);
    vt_pos(&s, VT_ENGINEER_ROW, VT_ENGINEER_COL);
    sputstr(&s, "Last: ");
    sputc(&s, group);
    sputint(&s, offset, 10);
    sputstr(&s, " Expect: ");
    sputint(&s, expected_time, 10);
    sputstr(&s, " Actual: ");
    sputint(&s, actual_time, 10);
    sputstr(&s, " Error: ");
    sputint(&s, error, 10);
    sputstr(&s, "         "); // to cover up the tail
    sputstr(&s, VT_CURSOR_RESTORE);
    PutString(COM2, &s);

}

void landmarkNotifier()
{
    int pid = MyParentTid();
    MessageToEngineer message;
    message.type = update_landmark;
    printf(COM2, "landmarkNotifier\r\n");
    printf(COM2, "landmarkNotifier\r\n");
    printf(COM2, "landmarkNotifier\r\n");
    printf(COM2, "landmarkNotifier\r\n");
    printf(COM2, "landmarkNotifier\r\n");
    printf(COM2, "landmarkNotifier\r\n");
    printf(COM2, "landmarkNotifier\r\n");

    for (;;)
    {
        int wake_time = 0;
        printf(COM2, "landmarkNotifier: sending to engineer tid %d\r\n", pid);
        Send(pid, &message, sizeof(MessageToEngineer), &wake_time, sizeof(wake_time));
        printf(COM2, "landmarkNotifier: delaying %d\r\n", wake_time);
        DelayUntil(wake_time);
    }
}

static void engineerTask() {
    Create(PRIORITY_ENGINEER_COURIER, engineerCourier);
    printf(COM2, "debug: engineerTask started >>> active_train %d desired_speed %d\r\n",
        active_train, desired_speed);
    trainSetSpeed(active_train, desired_speed);
    // at this point the train is up-to-speed
    int tid = 0, landmarkNotifierTid = 0;
    SensorUpdate last_update = {0,0};
    MessageToEngineer message;
    track_node *current_landmark = 0;

    Create(PRIORITY_ENGINEER_COURIER, landmarkNotifier);
    int current_velocity_in_um = 50; // TODO: maybe 50 is not good idea

    for(;;) {
        int len = Receive(&tid, &message, sizeof(MessageToEngineer));
        if(len != sizeof(MessageToEngineer)) {
            printf(COM2, "Warning engineer Received garbage\r\n");
            continue;
        }
        switch(message.type) {

            case x_mark: {
                printf(COM2, "engineer got x_mark\r\n");
                // if the engineer is approaching said sensor within, say 2 landmarks,
                // engineer should calculate when to send the stop command to stop on
                // top of the sensor

                // this calculation requires knowing the stopping distance.
                // we guess stopping distance for now
                // int stop_dist_mm = (desired_speed < 10) ? 300 : 600;

                // the current speed of travel
                // int current_speed = velocity[desired_speed];
                // calculate after which sensor should the engineer need to care about stopping
                // by backpropagating
                // int sensor_encoding = (sensor_group << 8) | sensor_number;
                // track_node *caring_sensor = backpropagateFrom(sensor_encoding, stopping_distance_in_mm);

                // tell the train engineer he needs to stop after x-time
                // int ticks_after_caring_sensor = 10000 * stopping_distance_in_cm / velocity_um_per_tick;
                // int distance_to_caring_sensor_in_um = umDistanceBetween(current_sensor, caring_sensor);
                // sum up number of ticks from our current sensor to the caring sensor by
                // looking up in the timing table
                // int num_ticks_from_now_to_caring_sensor = ;
                // when_to_wakeup_engineer_in_ticks =  ticks_after_caring_sensor +
                                        // num_ticks_from_now_to_caring_sensor;
                break;
            } // x_mark

            case change_speed: {
                printf(COM2, "engineer got change speed... \r\n");
                Reply(tid, 0, 0);
                desired_speed = message.data.change_speed.speed;
                trainSetSpeed(active_train, desired_speed);
                break;
            } // change_speed

            case update_landmark: {
                printf(COM2, "engineer got update_landmark... \r\n");
                printf(COM2, "engineer got update_landmark... \r\n");
                printf(COM2, "engineer got update_landmark... \r\n");
                printf(COM2, "engineer got update_landmark... \r\n");
                printf(COM2, "engineer got update_landmark... \r\n");
                printf(COM2, "engineer got update_landmark... \r\n");
                printf(COM2, "engineer got update_landmark... \r\n");
                printf(COM2, "engineer got update_landmark... \r\n");
                printf(COM2, "engineer got update_landmark... \r\n");
                printf(COM2, "engineer got update_landmark... \r\n");
                printf(COM2, "engineer got update_landmark... \r\n");
                printf(COM2, "engineer got update_landmark... \r\n");
                printf(COM2, "engineer got update_landmark... \r\n");
                // engineer should update screen with new estimated current_landmark
                // track_node *next_landmark = getNextLandmark(current_landmark);
                // int next_idx = next_landmark - &g_track[0];
                // current_landmark = &g_track[next_idx];
                // printf(COM2, "engineer got update_landmark: current_landmark was %s, setting it to %d",
                //     current_landmark->name, next_landmark->name);

                // if (next_landmark->type != NODE_SENSOR) {
                //     // we don't need a landmark notifier if
                //     // the next landmark is sensor, since
                //     // the engineer courier will notify us!

                //     // assume that landmark notifier is send blocked on us
                //     // at this point. If that is not the case, then it might
                //     // be delayed for too long previously

                //     track_edge *next_edge = getNextEdge(current_landmark);

                //     // calculate the time we want to landmark notifier to wake us up
                //     int expected_time_to_next_landmark = (next_edge->dist * 1000 / current_velocity_in_um);
                //     int wake_time = Time() + expected_time_to_next_landmark;
                //     printf(COM2, "replying to landmark notifier: expected_time_to_next_landmark %d wake_time %d",
                //         expected_time_to_next_landmark, wake_time);
                //     Reply(tid, &wake_time, sizeof(int));
                // }
                // else {
                //     printf(COM2, "next landmark is a sensor node, so no reply to notifier\r\n");
                //     landmarkNotifierTid = tid;
                // }

                break;
            } // update_landmark

            case update_sensor: {
                printf(COM2, ".");
                Reply(tid, 0, 0);

                SensorUpdate sensor_update = {
                    message.data.update_sensor.sensor,
                    message.data.update_sensor.time
                };

                // int group = sensor_update.sensor >> 8;
                // int offset = sensor_update.sensor & 0xff;
                // int index = indexFromSensorUpdate(sensor_update);
                // int new_time = sensor_update.time;
                // printf(COM2, "engineer read %c%d (%d idx) %d ticks\r\n",
                //          group + 'A', offset, index, new_time);


                // i. the real-time location of the train in form of landmark
                // for now define landmark as only sensor, then location is:

                int time_since_last_sensor = sensor_update.time - last_update.time;
                int um_dist_segment = umDistanceBetween(last_update, sensor_update);
                current_velocity_in_um = um_dist_segment / time_since_last_sensor;
                int est_dist_since_last_sensor = (current_velocity_in_um * time_since_last_sensor);

                // printf(COM2, "velocity %d um/tick, actual distance %d estimated distance %d\n\r",
                //     current_velocity_in_um,
                //     um_dist_segment,
                //     est_dist_since_last_sensor);


                // for amusement, display:
                // int average_velocity = length_of_track_circle / time_between_same_sensor
                // current_deviation_from_avg_v = abs(current_velocity - average_velocity)

                // ii. lookup the next landmark and display the estimate ETA to it


                int last_index = indexFromSensorUpdate(last_update);
                int index = indexFromSensorUpdate(sensor_update);
                char group = (sensor_update.sensor >> 8) + 'A';
                int offset = sensor_update.sensor & 0xff;
                int expected_time = last_update.time + pairs[last_index][index];
                int actual_time = sensor_update.time;
                int error = abs(expected_time - actual_time);
                updateScreen(group, offset, expected_time, actual_time, error);
                // printf(COM2, "Last: %c%d Exp: %d Act: %d Err: %d\r\n",
                    // group, offset, expected_time, actual_time, error);


                if(last_index >= 0) { // apply learning
                    int past_difference = pairs[last_index][index];
                    int new_difference = sensor_update.time - last_update.time;
                    pairs[last_index][index] =  (new_difference * ALPHA +
                                                past_difference * (100 - ALPHA)) / 100;
                    // printf(COM2, "time updated %d to %d \r\n",
                        // past_difference, pairs[last_index][index]);
                }

                last_index = index;
                last_update.time = sensor_update.time;
                last_update.sensor = sensor_update.sensor;


                // update the current_landmark
                current_landmark = &g_track[indexFromSensorUpdate(sensor_update)];

                // look up the next landmark
                track_node *next_landmark = getNextLandmark(current_landmark);
                // if (next_landmark->type != NODE_SENSOR) {
                //     // we don't need a landmark notifier if
                //     // the next landmark is sensor, since
                //     // the engineer courier will notify us!

                //     // assume that landmark notifier is send blocked on us
                //     // at this point. If that is not the case, then it might
                //     // be delayed for too long previously
                //     // assert(landmarkNotifierTid > 0);

                //     track_edge *next_edge = getNextEdge(current_landmark);

                //     // calculate the time we want to landmark notifier to wake us up
                //     int expected_time_to_next_landmark = (next_edge->dist * 1000 / current_velocity_in_um);
                //     int wake_time = sensor_update.time + expected_time_to_next_landmark;
                //     printf(COM2, "replying to landmark notifier: expected_time_to_next_landmark %d wake_time %d",
                //         expected_time_to_next_landmark, wake_time);
                //     Reply(landmarkNotifierTid, &wake_time, sizeof(int));
                //     landmarkNotifierTid = -1;
                // }

                break;
            } // update

            default: {
                printf(COM2, "Warning engineer Received garbage\r\n");
                break;
            } // default
        } // switch

                // print out the direction of travel


    } // for

} // engineerTask

static int engineerTaskId = -1;

void initEngineer() {
    // if(engineerTaskId >= 0) {
        // we already called initEngineer before
        // return;
    // }
    printf(COM2, "debug: initEngineer\r\n");
    for(int i = 0; i < NUM_SENSORS; i++) {
        for(int j = 0; j < NUM_SENSORS; j++) {
            pairs[i][j] = 0;
        }
    }
    // for(int i = 0; i < NUM_TRAINS; i++) {
        // activated_engineers[i] = false;
    // }
    direction_is_forward = true;
    engineerTaskId = Create(PRIORITY_ENGINEER, engineerTask);
    assert(engineerTaskId >= 0);
}

void engineerPleaseManThisTrain(int train_number, int speed) {
    assert(1 <= train_number && train_number <= 80 && 0 <= speed && speed <= 14);
    // if(activated_engineers[train_number - 1]) {
        // printf(COM2, "Engineer for %d is already started", train_number);
        // return;
    // }
    // activated_engineers[train_number - 1] = true;
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

track_node *getPrevLandmark(track_node *node) {

    return (track_node *) 0;
}

// return the track node from which the engineer should start caring about
track_node *backpropagateFrom(int sensorEncoding, int stopping_distance_in_cm) {
    int sensor = 16 * (sensorEncoding >> 8) + (sensorEncoding & 0xff) - 1;
    assert (sensor >= 0 && sensor < NUM_SENSORS);

    return (track_node *) 0;
}

static int when_to_wakeup_engineer_in_ticks;
// void clockwaiter() {
//     int tid;
//     Wait
//     for(;;) {
//         Receive(&tid, 0, 0);
//         DelayUntil(when_to_wakeup_engineer_in_ticks);
//         assert(engineerTaskId >= 0);
//         int wakeupMessage = WAKEUP;
//         Send(engineerTaskId, &wakeupMessage, sizeof(int), 0, 0);

//     }
// }

void engineerXMarksTheSpot(char sensor_group, int sensor_number) {
    assert('a' <= sensor_group && sensor_group <= 'e');
    assert(1 <= sensor_number && sensor_number <= 16);
    assert(engineerTaskId >= 0);
    MessageToEngineer message;
    message.type = x_mark;
    message.data.x_mark.x_sensor = (sensor_group << 8) | sensor_number;
    Send(engineerTaskId, &message, sizeof(MessageToEngineer), 0, 0);
}

void engineerSpeedUpdate(int new_speed) {
    MessageToEngineer message;
    message.type = change_speed;
    message.data.change_speed.speed = new_speed;
    assert(engineerTaskId >= 0);
    Send(engineerTaskId, &message, sizeof(MessageToEngineer), 0, 0);
}
