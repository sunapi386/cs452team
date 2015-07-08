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

#define UI_REFRESH_INTERVAL 5
#define UI_MESSAGE_INIT     20
#define UI_MESSAGE_DIST     21
#define UI_MESSAGE_LANDMARK 22
#define UI_MESSAGE_SENSOR   23
#define UI_MESSAGE_RESTART  24

typedef struct UIMessage {
    int type;
    int error;
    int distToNext;
    int expectedTime;
    int actualTime;
    const char *prevSensor;
    const char *nextLandmark;
    const char *prevLandmark;
} UIMessage;

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

static inline int indexFromSensorUpdate(SensorUpdate *update) {
    return 16 * (update->sensor >> 8) + (update->sensor & 0xff) - 1;
}

int distanceBetween(track_node *from, track_node *to) {
    assert(from != 0);
    assert(to != 0);

    int total_distance = 0;
    int i;
    track_node *next_node = from;
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
        if(next_node == to) {
            // printf(COM2, "\r\ndistanceBetween: total_distance %d.\n\r", total_distance);
            break;
        }
    }
    if(i == TRACK_MAX) {
        printf(COM2, "distanceBetween: something went wrong: %d.\n\r", i);
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

void initializeScreen()
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
    // convert from um to cm
    printf(COM2, VT_CURSOR_SAVE
                 "\033[3;100H%d.%d cm     "
                 VT_CURSOR_RESTORE,
                 distToNext / 10000,
                 abs(distToNext) % 10000);
}

void updateScreenNewLandmark(const char *nextLandmark,
                             const char *prevLandmark,
                             int distToNext)
{
    printf(COM2, VT_CURSOR_SAVE
                 "\033[1;100H%s    "
                 "\033[2;100H%s    "
                 "\033[3;100H%d.%d cm     "
                 VT_CURSOR_RESTORE,
                 nextLandmark,
                 prevLandmark,
                 distToNext / 10000,
                 abs(distToNext) % 10000);
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
                 "\033[3;100H%d.%d cm     "
                 "\033[4;100H%s    "
                 "\033[5;100H%d    "
                 "\033[6;100H%d    "
                 "\033[7;100H%d    "
                 VT_CURSOR_RESTORE,
                 nextLandmark,
                 prevLandmark,
                 distToNext / 10000,
                 abs(distToNext) % 10000,
                 prevSensor,
                 expectedTime,
                 actualTime,
                 error);
}

void UIWorker()
{
    int pid = MyParentTid();

    UIMessage uiMessage;
    MessageToEngineer engMessage;
    engMessage.type = update_location;

    for (;;)
    {
        // receive data to display
        int len = Send(pid, &engMessage, sizeof(engMessage), &uiMessage, sizeof(uiMessage));
        assert(len == sizeof(uiMessage));

        // update display
        switch (uiMessage.type)
        {
        case UI_MESSAGE_DIST:
            updateScreenDistToNext(uiMessage.distToNext);
            break;
        case UI_MESSAGE_LANDMARK:
            updateScreenNewLandmark(uiMessage.nextLandmark,
                                    uiMessage.prevLandmark,
                                    uiMessage.distToNext);
            break;
        case UI_MESSAGE_SENSOR:
            updateScreenNewSensor(uiMessage.nextLandmark,
                                  uiMessage.prevLandmark,
                                  uiMessage.distToNext,
                                  uiMessage.prevSensor,
                                  uiMessage.expectedTime,
                                  uiMessage.actualTime,
                                  uiMessage.error);

            break;
        case UI_MESSAGE_INIT:
            initializeScreen();
            break;
        case UI_MESSAGE_RESTART:
            break;
        default:
            printf(COM2, "UIWorker received garbage\n\r");
            assert(0);
            break;
        }

        // delay until the next update
        Delay(UI_REFRESH_INTERVAL);
    }
}

static void engineerTask() {
    trainSetSpeed(active_train, desired_speed);

    // Variables that are needed to be persisted between iterations
    int tid = 0, sensorReadingCount = 0;
    int uiWorkerTid = 0, shouldRefreshSensor = 0;
    int current_velocity_in_um = 50; // TODO: maybe 50 is not good idea
    int timeOfReachingPrevLandmark = 0;
    track_node *prevLandmark = 0;
    SensorUpdate last_update = {0,0};

    // Stopping distance related
    track_node *targetLandmark = 0;
    int forwardStoppingDist[15] = {0, 100, 300, 600, 900, 1100, 2500, 4000, 5000, 6000, 6000, 1450000, 1250000, 1250000, 1250000};
    int backwardStoppingDist[15] = {0, 100, 300, 600, 900, 1100, 2500, 4000, 5000, 6000, 6000, 1450000, 1250000, 1250000, 1250000};

    UIMessage uiMessage;
    MessageToEngineer message;

    // Create child tasks
    Create(PRIORITY_ENGINEER_COURIER, UIWorker);
    Create(PRIORITY_ENGINEER_COURIER, engineerCourier);

    // All sensor reports with timestamp before this time are invalid
    int timeAtReceive = Time();

    for(;;) {
        int len = Receive(&tid, &message, sizeof(MessageToEngineer));
        assert(len == sizeof(MessageToEngineer));

        switch(message.type) {
            case update_location:
            {
                // If we are just starting, initialize display and block
                if (prevLandmark == 0)
                {
                    uiWorkerTid = tid;
                    break;
                }

                // If sensor was just hit, reply directly to update last sensor
                if (shouldRefreshSensor)
                {
                    Reply(tid, &uiMessage, sizeof(uiMessage));
                    shouldRefreshSensor = 0;
                    break;
                }

                // If we have a prevLandmark, compute the values needed
                // and update train display
                track_node *nextLandmark = getNextLandmark(prevLandmark);

                // If landmark is null or train is not moving, do not refresh UI
                if (nextLandmark == 0 || desired_speed == 0)
                {
                    uiWorkerTid = tid;
                    break;
                }

                // get the edge from previous to next landmark
                track_edge *nextEdge = getNextEdge(prevLandmark);
                if (nextEdge == 0)
                {
                    assert(0); // TODO (shuo): remove
                    Reply(tid, 0, 0);
                    continue;
                }

                // compute some stuff
                int currentTime = Time();
                int delta = currentTime - timeOfReachingPrevLandmark;
                int distSoFar = delta * current_velocity_in_um;
                int distToNextLandmark = nextEdge->dist * 1000 - distSoFar;

                // check for X
                if (direction_is_forward && targetLandmark != 0 &&
                    (distanceBetween(prevLandmark, targetLandmark) - distSoFar
                    <= forwardStoppingDist[desired_speed]) )   {
                    // issue stop command now
                    desired_speed = 0;
                    trainSetSpeed(active_train, desired_speed);
                }// check for X
                else if (! direction_is_forward && targetLandmark != 0 &&
                    (distanceBetween(prevLandmark, targetLandmark) - distSoFar
                    <= backwardStoppingDist[desired_speed]) )   {
                    // issue stop command now
                    desired_speed = 0;
                    trainSetSpeed(active_train, desired_speed);
                }

                if (nextLandmark->type == NODE_SENSOR)
                {
                    // output the distance to next landmark
                    uiMessage.type = UI_MESSAGE_DIST;
                    uiMessage.distToNext = distToNextLandmark;
                    Reply(tid, &uiMessage, sizeof(uiMessage));
                }
                else
                {
                    if (distToNextLandmark <= 0)
                    {
                        // We've "reached" the next non-sensor landmark
                        // Update previous landmark
                        prevLandmark = nextLandmark;
                        timeOfReachingPrevLandmark = currentTime;

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
                        distToNextLandmark = nextEdge->dist * 1000;

                        // Output: next landmark, prev landmark, dist to next
                        uiMessage.type = UI_MESSAGE_LANDMARK;
                        uiMessage.nextLandmark = nextLandmark->name;
                        uiMessage.prevLandmark = prevLandmark->name;
                        uiMessage.distToNext = distToNextLandmark;
                        Reply(tid, &uiMessage, sizeof(uiMessage));

                    }
                    else
                    {
                        // We haven't reached the next landmark;
                        // Output: distance to next (already computed)
                        uiMessage.type = UI_MESSAGE_DIST;
                        uiMessage.distToNext = distToNextLandmark;
                        Reply(tid, &uiMessage, sizeof(uiMessage));
                    }
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
                prevLandmark = &g_track[indexFromSensorUpdate(&sensor_update)];
                timeOfReachingPrevLandmark = sensor_update.time;

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
                int um_dist_segment = distanceBetween(&g_track[indexFromSensorUpdate(&last_update)],
                                                      &g_track[indexFromSensorUpdate(&sensor_update)]);
                current_velocity_in_um = um_dist_segment / time_since_last_sensor;

                // update constant velocity calibration data
                int last_index = indexFromSensorUpdate(&last_update);
                int index = indexFromSensorUpdate(&sensor_update);

                int expected_time = last_update.time + pairs[last_index][index];
                int actual_time = sensor_update.time;
                int error = abs(expected_time - actual_time);

                if(last_index >= 0) { // apply learning
                    int past_difference = pairs[last_index][index];
                    int new_difference = sensor_update.time - last_update.time;
                    pairs[last_index][index] =  (new_difference * ALPHA +
                                                past_difference * (100 - ALPHA)) / 100;

                    int distanceToNextLandmark = nextEdge->dist * 1000;

                    // output prevLandmark, nextLandmark, distance to next,
                    // previous sensor, expected time, actual time, error
                    uiMessage.type = UI_MESSAGE_SENSOR;
                    uiMessage.nextLandmark = nextLandmark->name;
                    uiMessage.prevLandmark = prevLandmark->name;
                    uiMessage.distToNext = distanceToNextLandmark;
                    uiMessage.prevSensor = prevLandmark->name;
                    uiMessage.expectedTime = expected_time;
                    uiMessage.actualTime = actual_time;
                    uiMessage.error = error;
                    shouldRefreshSensor = 1;
                }

                last_update.time = sensor_update.time;
                last_update.sensor = sensor_update.sensor;

                if (sensorReadingCount == 0 && uiWorkerTid > 0)
                {
                    // kick start UI refresh
                    uiMessage.type = UI_MESSAGE_INIT;
                    Reply(uiWorkerTid, &uiMessage, sizeof(uiMessage));
                    ++sensorReadingCount;
                }

                // unblock sensor courier
                Reply(tid, 0, 0);

                break;
            } // update

            case x_mark: {
                assert(message.data.x_mark.x_node_number >= 0 &&
                    message.data.x_mark.x_node_number < TRACK_MAX);

                targetLandmark = &g_track[message.data.x_mark.x_node_number];
                Reply(tid, 0, 0);
                break;
            } // x_mark

            case change_speed: {
                // printf(COM2, "engineer got change speed... \r\n");
                Reply(tid, 0, 0);
                desired_speed = message.data.change_speed.speed;
                targetLandmark = 0;
                trainSetSpeed(active_train, desired_speed);
                if (uiWorkerTid > 0) {
                    Reply(uiWorkerTid, &uiMessage, sizeof(uiMessage));
                    uiWorkerTid = 0;
                }

                break;
            } // change_speed

            case reverse_direction: {
                Reply(tid, 0, 0);
                targetLandmark = 0;
                direction_is_forward = (! direction_is_forward);
                printf(COM2, "trainSetReverseNicely %d\r\n", active_train);
                // trainSetReverseNicely(active_train);
                timeOfReachingPrevLandmark = 0;
                prevLandmark = 0;
                last_update.time = 0;
                last_update.sensor = 0;
                break;
            }
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
    // printf(COM2, "debug: initEngineer\r\n");
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
    MessageToEngineer message;
    message.type = reverse_direction;
    Send(engineerTaskId, &message, sizeof(MessageToEngineer), 0, 0);
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
