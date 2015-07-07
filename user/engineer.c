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

#define ALPHA               85
#define NUM_SENSORS         (5 * 16)
#define NUM_TRAINS          80
#define UI_REFRESH_INTERVAL 10

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
        if(next_node->idx == to_index) {
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
        printf(COM2, "gne?");
    }
    return 0;

}

// function that looks up the next landmark, using direction_is_forward
// returns a landmark
track_node *getNextLandmark(track_node *current_landmark) {
    track_edge *next_edge = getNextEdge(current_landmark);
    return (next_edge == 0 ? 0 : next_edge->dest);
}

static bool direction_is_forward = true;

void initializeEngineerDisplay()
{
    printf(COM2, VT_CURSOR_SAVE
                 "\033[1;80HNext landmark:     "
                 "\033[2;80HPrevious landmark: "
                 "\033[3;80HDistance to next:  "
                 "\033[4;80HPrevious sensor:   "
                 "\033[5;80HExpected (ticks):  "
                 "\033[6;80HActual   (ticks):  "
                 "\033[7;80HError    (ticks):  "
                 VT_CURSOR_RESTORE);
}

void updateScreenDistToNext(int distToNext)
{
    printf(COM2, VT_CURSOR_SAVE
                 "\033[3;100H%d    "
                 VT_CURSOR_RESTORE,
                 distToNext);
}

void updateScreenNewLandmark(const char *nextLandmark,
                             const char *prevLandmark,
                             int distToNext)
{
    printf(COM2, VT_CURSOR_SAVE
                 "\033[1;100H%s    "
                 "\033[2;100H%s    "
                 "\033[3;100H%d    "
                 VT_CURSOR_RESTORE,
                 nextLandmark,
                 prevLandmark,
                 distToNext);
}

void updateScreenNewSensor(const char *nextLandmark,
                           const char *prevLandmark,
                           int distToNext,
                           const char *prevSensor,
                           int expectedTime,
                           int actualTime,
                           int error)
{
    printf(COM2, VT_CURSOR_SAVE
                 "\033[1;100H%s    "
                 "\033[2;100H%s    "
                 "\033[3;100H%d    "
                 "\033[4;100H%s    "
                 "\033[5;100H%d    "
                 "\033[6;100H%d    "
                 "\033[7;100H%d    "
                 VT_CURSOR_RESTORE,
                 nextLandmark,
                 prevLandmark,
                 distToNext,
                 prevSensor,
                 expectedTime,
                 actualTime,
                 error);
}


void refreshNotifier()
{
    int pid = MyParentTid();
    MessageToEngineer message;
    message.type = update_ui;

    for (;;)
    {
        Send(pid, &message, sizeof(MessageToEngineer), 0, 0);
        Delay(UI_REFRESH_INTERVAL); // update UI every 10 seconds
    }
}

static void engineerTask() {
    printf(COM2, "debug: engineerTask started >>> active_train %d desired_speed %d\r\n",
        active_train, desired_speed);
    trainSetSpeed(active_train, desired_speed);

    // Variables that are needed to be persisted between iterations
    int tid = 0, refreshNotifierTid = 0, sensorReadingCount = 0;
    int current_velocity_in_um = 50; // TODO: maybe 50 is not good idea

    int timeOfReachingPrevLandmark = 0;
    track_node *prevLandmark = 0;

    int expectedSensorTime = 0;
    int actualSensorTime = 0;

    SensorUpdate last_update = {0,0};
    MessageToEngineer message;

    // Create child tasks
    Create(PRIORITY_ENGINEER_COURIER, refreshNotifier);
    Create(PRIORITY_ENGINEER_COURIER, engineerCourier);

    // All sensor reports with timestamp before this time are invalid
    int timeAtReceive = Time();

    for(;;) {
        int len = Receive(&tid, &message, sizeof(MessageToEngineer));
        assert(len == sizeof(MessageToEngineer));

        switch(message.type) {
            case update_ui:
            {
                // If we are just starting, initialize display and block
                if (prevLandmark == 0)
                {
                    initializeEngineerDisplay();
                    refreshNotifierTid = tid;
                    break;
                }

                // If we have a prevLandmark, compute the values needed
                // and update train display
                track_node *nextLandmark = getNextLandmark(prevLandmark);

                // If landmark is null or train is not moving, do not refresh UI
                if (nextLandmark == 0 || desired_speed == 0)
                {
                    refreshNotifierTid = tid;
                    break;
                }
                // If the next landmark is a sensor, we know:
                //      1. We have not reached the next landmark yet
                // We need to do:
                //      1. compute the distance to next landmark (even it becomes negative)
                //      2. Output distance to next landmark
                else if (nextLandmark->type == NODE_SENSOR)
                {
                    /*
                        Compute the distance to next landmark
                    */

                    // get the edge from previous to next landmark
                    track_edge *nextEdge = getNextEdge(prevLandmark);
                    if (nextEdge == 0)
                    {
                        assert(0); // TODO (shuo): remove
                        Reply(tid, 0, 0);
                        continue;
                    }

                    // delta is the difference between now and when we reached last landmark
                    int delta = Time() - timeOfReachingPrevLandmark;
                    assert(delta >= 0);

                    // compute distance to next landmark
                    int distToNextLandmark = nextEdge->dist - delta * current_velocity_in_um;

                    /*
                        Output the distance to next landmark
                    */

                    // print to screen
                    updateScreenDistToNext(distToNextLandmark);

                    // unblock the notifier
                    Reply(tid, 0, 0);
                }
                // If the next landmark is a non-sensor, we know:
                //      1. We may/may not have reached the next landmark
                // Therefore, we need to do:
                //      1. Compute the time difference from now to the time of reaching the previous landmark
                //      2. Using that, compute the distance until we reach the next landmark
                //      3. If that distance is <= 0
                //          a. we know we've reached the next landmark
                //          b. we update prevLandmark, set distance to next to 0, set time of reaching previous to 0
                //          c. we update the screen for next landmark, prev landmark, distance to next
                //          d. we unblock the refreshNotifier
                //      4. Else if that distance is positive
                //          a. we know we haven't reached the next landmark
                //          b. we update screen for distance to next
                else
                {
                    /*
                        Compute some stuff to decide exactly what we need to do here
                    */

                    // get the edge from previous to next landmark
                    track_edge *nextEdge = getNextEdge(prevLandmark);
                    if (nextEdge == 0)
                    {
                        assert(0); // TODO (shuo): remove
                        Reply(tid, 0, 0);
                        continue;
                    }

                    // delta is the difference between now and when we reached last landmark
                    int delta = Time() - timeOfReachingPrevLandmark;
                    assert(delta >= 0);

                    // compute distance to next landmark
                    int distToNextLandmark = nextEdge->dist - delta * current_velocity_in_um;

                    if (distToNextLandmark <= 0)
                    {
                        // We've "reached" the next non-sensor landmark
                        // Update previous landmark
                        prevLandmark = nextLandmark;
                        timeOfReachingPrevLandmark = 0;

                        // update the next landmark
                        nextLandmark = getNextLandmark(prevLandmark);
                        track_edge *nextEdge = getNextEdge(prevLandmark);
                        if (nextLandmark == 0 || nextEdge == 0)
                        {
                            assert(0); // TODO (shuo): remove
                            Reply(tid, 0, 0);
                            continue;
                        }

                        // update the distance to next landmark
                        distToNextLandmark = nextEdge->dist;

                        // Output: next landmark, prev landmark, dist to next
                        updateScreenNewLandmark(nextLandmark->name, prevLandmark->name, distToNextLandmark);
                    }
                    else
                    {
                        // We haven't reached the next landmark;
                        // Output: distance to next (already computed)
                        updateScreenDistToNext(distToNextLandmark);
                    }

                    Reply(tid, 0, 0);
                }

                break;
            }

            case update_sensor: {
                /*
                    If we are here, we know:
                        1. A sensor has been triggered;
                        2. We've received a sensor update, consists of a sensor and a time stamp
                    We need to update:
                        1. Previous landmark
                        2. Next landmark
                        3. Distance to next landmark
                        4. Previous sensor
                        5. Expected time
                        6. Actual time
                        7. Error
                */

                // Get sensor update data
                SensorUpdate sensor_update = {
                    message.data.update_sensor.sensor,
                    message.data.update_sensor.time
                };

                // Reject sensor updates with timestamp before timeAtReceive
                if (sensor_update.time <= timeAtReceive)
                {
                    Reply(tid, 0, 0);
                    continue;
                }

                // Update some stuff
                prevLandmark = &g_track[indexFromSensorUpdate(sensor_update)];
                track_node *nextLandmark = getNextLandmark(prevLandmark);
                track_edge *nextEdge = getNextEdge(prevLandmark);
                if (nextLandmark == 0 || nextEdge == 0)
                {
                    assert(0); // TODO (shuo): remove
                    Reply(tid, 0, 0);
                    continue;
                }

                // update the current velocity
                int time_since_last_sensor = sensor_update.time - last_update.time;
                int um_dist_segment = umDistanceBetween(last_update, sensor_update);
                current_velocity_in_um = um_dist_segment / time_since_last_sensor;

                // update constant velocity calibration data
                int last_index = indexFromSensorUpdate(last_update);
                int index = indexFromSensorUpdate(sensor_update);

                int expected_time = last_update.time + pairs[last_index][index];
                int actual_time = sensor_update.time;
                int error = abs(expected_time - actual_time);

                if(last_index >= 0) { // apply learning
                    int past_difference = pairs[last_index][index];
                    int new_difference = sensor_update.time - last_update.time;
                    pairs[last_index][index] =  (new_difference * ALPHA +
                                                past_difference * (100 - ALPHA)) / 100;

                    // output prevLandmark, nextLandmark, distance to next,
                    // previous sensor, expected time, actual time, error
                    updateScreenNewSensor(nextLandmark->name,
                                          prevLandmark->name,
                                          nextEdge->dist,
                                          prevLandmark->name,
                                          expected_time,
                                          actual_time,
                                          error);
                }

                last_update.time = sensor_update.time;
                last_update.sensor = sensor_update.sensor;

                if (sensorReadingCount == 0 && refreshNotifierTid > 0)
                {
                    // kick start UI refresh
                    Reply(refreshNotifierTid, 0, 0);
                    ++sensorReadingCount;
                }

                // unblock sensor courier
                Reply(tid, 0, 0);

                break;
            } // update

            case x_mark: {
                int node_number = message.data.x_mark.x_node_number;
                Reply(tid, 0, 0);
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
                // printf(COM2, "engineer got change speed... \r\n");
                Reply(tid, 0, 0);
                desired_speed = message.data.change_speed.speed;
                trainSetSpeed(active_train, desired_speed);
                break;
            } // change_speed

            default: {
                printf(COM2, "Warning engineer Received garbage\r\n");
                assert(0);
                break;
            } // default
        } // switch
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

void engineerXMarksTheSpot(int node_number) {
    assert(0 <= node_number && node_number <= 139);
    assert(engineerTaskId >= 0);
    MessageToEngineer message;
    message.type = x_mark;
    message.data.x_mark.x_node_number = node_number;
    Send(engineerTaskId, &message, sizeof(MessageToEngineer), 0, 0);
}

void engineerSpeedUpdate(int new_speed) {
    MessageToEngineer message;
    message.type = change_speed;
    message.data.change_speed.speed = new_speed;
    assert(engineerTaskId >= 0);
    Send(engineerTaskId, &message, sizeof(MessageToEngineer), 0, 0);
}
