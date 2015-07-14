#include <user/engineer.h>
#include <priority.h>
#include <debug.h> // assert
#include <user/syscall.h>
#include <utils.h> // printf
#include <user/train.h> // trainSetSpeed
#include <user/sensor.h>
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
#define UI_MESSAGE_PASS     24

#define COMMAND_SET_SPEED   30
#define COMMAND_REVERSE     31

typedef struct {
    char type;
    char turnoutDir;
    char trainSpeed;
    char trainNumber;
    char turnoutNumber;
} Command;

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

typedef enum {
    Init = 0,           // initial state
    Stop,               // stop command issued
    Running,            // running
    ReverseSetReverse,  // reverse stage 2: reverse issued
} TrainState;

static int desired_speed = -1;
static int active_train = -1; // static for now, until controller is implemented
static int pairs[NUM_SENSORS][NUM_SENSORS];
static track_node g_track[TRACK_MAX]; // This is guaranteed to be big enough.
static int engineerTaskId = -1;

static inline int abs(int num) {
    return (num < 0 ? -1 * num : num);
}

// static bool activated_engineers[NUM_TRAINS];

// Courier: sensorServer -> engineer
static void engineerCourier() {
    int engineer = MyParentTid();
    int sensor = WhoIs("sensorServer");

    SensorRequest sensorReq;  // courier <-> sensorServer
    sensorReq.type = MESSAGE_ENGINEER_COURIER;
    SensorUpdate result;  // courier <-> sensorServer
    EngineerMessage engineerReq; // courier <-> engineer
    engineerReq.type = updateSensor;

    for (;;) {
        Send(sensor, &sensorReq, sizeof(sensorReq), &result, sizeof(result));
        engineerReq.data.updateSensor.sensor = result.sensor;
        engineerReq.data.updateSensor.time = result.time;
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

track_edge *getNextEdge(track_node *landmark)
{
    if(landmark->type == NODE_BRANCH)
    {
        return &landmark->edge[turnoutIsCurved(landmark->num)];
    }
    else if (landmark->type == NODE_EXIT)
    {
        return 0;
    }
    return &landmark->edge[DIR_AHEAD];
}

track_node *getNextLandmark(track_node *current_landmark) {
    track_edge *next_edge = getNextEdge(current_landmark);
    return (next_edge == 0 ? 0 : next_edge->dest);
}

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
    EngineerMessage engMessage;
    engMessage.type = updateLocation;

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
        case UI_MESSAGE_PASS:
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

void commandWorker()
{
    int pid = MyParentTid();

    EngineerMessage message;
    message.type = commandWorker;

    Command command;

    for (;;)
    {
        // Send to engineer to get new command
        message.type = commandWorker;
        int len = Send(pid, &message, sizeof(message), &command, sizeof(command));
        assert(len == sizeof(Command));

executeCommand:
        switch(command.type)
        {
        case COMMAND_SET_SPEED:
            trainSetSpeed(command.trainNumber, command.trainSpeed);

            // Notify the engineer that the set speed command has been issued
            message.type = commandWorkerSetSpeed;
            message.data.setSpeed.speed = command.trainSpeed;
            Send(pid, &message, sizeof(message), 0, 0);
            break;
        case COMMAND_REVERSE:
            trainSetSpeed(command.trainNumber, 0);

            // Notify the engineer that the set speed command has been issued
            message.type = commandWorkerSetSpeed;
            message.data.setSpeed.speed = 0;

            // Wait until train comes to a stop and set reverse
            Delay(300); // TODO: scale this
            trainSetReverse(command.trainNumber);

            // Notify the engineer that the reverse command has been issued
            message.type = commandWorkerSetReverse;
            Send(pid, &message, sizeof(message), 0, 0);

            // Set the train back to original speed
            Delay(15);
            goto executeCommand;
            break;
        default:
            printf(COM2, "[commandWorker] Invalid command\n\r");
            assert(0);
        }
    }
}

void engineerTask() {
    int tid = 0;
    int uiWorkerTid = 0;
    int commandWorkerTid = 0;

    bool isForward = true;
    char speed = 0;   // 0-14
    int velocity = 0;         // um/tick
    int triggeredSensor = 0;  // {0|1}
    int prevLandmarkTime = 0; // tick

    TrainState state = Init;
    track_node *prevLandmark = 0;
    SensorUpdate last_update = {0,0};
    track_node *targetLandmark = 0;
    int forwardStoppingDist[15] = {0, 100, 300, 600, 900, 1100, 2500, 4000, 5000, 6000, 6000, 1450000, 1250000, 948200,  948200};
    int backwardStoppingDist[15] = {0, 100, 300, 600, 900, 1100, 2500, 4000, 5000, 6000, 6000, 1450000, 1250000, 948200, 1000000};

    Command cmdBuf[32];
    CommandQueue commandQueue;
    InitCommandQueue(&commandQueue, 32, &(cmdBuf[0]));

    UIMessage uiMessage;
    EngineerMessage message;

    // Create child tasks
    Create(PRIORITY_ENGINEER_COURIER, engineerCourier);
    Create(PRIORITY_ENGINEER_COURIER, commandWorker);
    uiWorkerTid = Create(PRIORITY_ENGINEER_COURIER, UIWorker);

    // All sensor reports with timestamp before this time are invalid
    int creationTime = Time();

    for(;;)
    {
        int len = Receive(&tid, &message, sizeof(EngineerMessage));
        assert(len == sizeof(EngineerMessage));

        switch(message.type)
        {
            case updateLocation:
            {
                if (state == Init)
                {
                    // If we are just starting, block until we reach the first sensor
                    break;
                }

                if (triggeredSensor)
                {
                    // If sensor was just hit, reply directly to update last sensor
                    Reply(tid, &uiMessage, sizeof(uiMessage));
                    triggeredSensor = 0;
                    break;
                }

                assert(prevLandmark != 0);

                if (state == )

                // If we have a prevLandmark, compute the values needed
                // and update train display
                track_node *nextLandmark = getNextLandmark(prevLandmark);
                track_edge *nextEdge = getNextEdge(prevLandmark);
                if (nextEdge != 0 || nextLandmark != 0)
                {
                    printf(COM2, "[engineer:321] Warning: nextEdge/nextLandmark is NULL");
                    uiWorkerTid = tid;
                    break;
                }

                // compute some stuff
                int currentTime = Time();
                int delta = currentTime - prevLandmarkTime;
                int distSoFar = delta * velocity;
                int distToNextLandmark = nextEdge->dist * 1000 - distSoFar;

                // check for stopping at landmark
                if (direction == Forward && targetLandmark != 0 &&
                    (distanceBetween(prevLandmark, targetLandmark) - distSoFar
                    <= forwardStoppingDist[desired_speed]) )   {
                    targetLandmark = 0;
                    desired_speed = 0;
                    trainSetSpeed(active_train, desired_speed);
                }
                else if (direction == Backward && targetLandmark != 0 &&
                    (distanceBetween(prevLandmark, targetLandmark) - distSoFar
                    <= backwardStoppingDist[desired_speed]) )   {
                    targetLandmark = 0;
                    desired_speed = 0;
                    trainSetSpeed(active_train, desired_speed);
                }

                if (nextLandmark->type == NODE_SENSOR)
                {
                    if (state == ReverseSetReverse)
                    {
                        // swap the prevLandmark with nextLandmark
                        track_node *temp = nextLandmark;
                        nextLandmark = prevLandmark;
                        prevLandmark = temp;

                        // swap the distSoFar with distToNextLandmark
                    }

                    // output the distance to next landmark
                    uiMessage.type = UI_MESSAGE_DIST;
                    uiMessage.distToNext = distToNextLandmark;
                    Reply(tid, &uiMessage, sizeof(uiMessage));
                }
                else if (distToNextLandmark <= 0)
                {
                    // We've "reached" the next non-sensor landmark
                    // Update previous landmark
                    prevLandmark = nextLandmark;
                    prevLandmarkTime = currentTime;

                    // update the next landmark
                    nextLandmark = getNextLandmark(prevLandmark);
                    track_edge *nextEdge = getNextEdge(prevLandmark);
                    if (nextLandmark == 0 || nextEdge == 0)
                    {
                        uiWorkerTid = tid;
                        printf(COM2, "[engineer:378] Warning: nextEdge/nextLandmark is NULL");
                        break;
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
                else // (distanceToNextLandmark > 0)
                {
                    // We haven't reached the next landmark;
                    // Output: distance to next (already computed)
                    uiMessage.type = UI_MESSAGE_DIST;
                    uiMessage.distToNext = distToNextLandmark;
                    Reply(tid, &uiMessage, sizeof(uiMessage));
                }
                break;
            }

            case updateSensor: {
                // unblock sensor
                Reply(tid, 0, 0);

                // Get sensor update data
                SensorUpdate sensor_update = {
                    message.data.updateSensor.sensor,
                    message.data.updateSensor.time
                };

                // Reject sensor updates with timestamp before creationTime
                if (sensor_update.time <= creationTime) continue;

                // Update some stuff
                prevLandmark = &g_track[indexFromSensorUpdate(&sensor_update)];
                prevLandmarkTime = sensor_update.time;

                if (uiWorkerTid > 0)
                {
                    uiMessage.type = UI_MESSAGE_PASS;
                    Reply(uiWorkerTid, &uiMessage, sizeof(uiMessage));
                    uiWorkerTid = 0;
                }

                track_node *nextLandmark = getNextLandmark(prevLandmark);
                track_edge *nextEdge = getNextEdge(prevLandmark);
                assert(nextLandmark && nextEdge);

                // update the current velocity
                int time_since_last_sensor = sensor_update.time - last_update.time;
                int um_dist_segment = distanceBetween(&g_track[indexFromSensorUpdate(&last_update)],
                                                      &g_track[indexFromSensorUpdate(&sensor_update)]);
                velocity = um_dist_segment / time_since_last_sensor;

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
                    triggeredSensor = 1;
                }

                last_update.time = sensor_update.time;
                last_update.sensor = sensor_update.sensor;

                if (state == Init)
                {
                    state = speed ? Running : Stop;

                    // kick start UI refresh
                    uiMessage.type = UI_MESSAGE_INIT;
                    Reply(uiWorkerTid, &uiMessage, sizeof(uiMessage));
                    uiWorkerTid = 0;
                }
                break;
            }

            case xMark:
            {
                Reply(tid, 0, 0);
                targetLandmark = &g_track[message.data.xMark.nodeNumber];
                break;
            }

            case setSpeed:
            {
                Reply(tid, 0, 0);

                Command command = {
                    .type = COMMAND_SET_SPEED,
                    .speed = (char)(message.data.setSpeed.speed)
                };

                if (commandWorkerTid > 0)
                {
                    Reply(commandWorkerTid, &command, sizeof(command));
                    commandWorkerTid = 0;
                }
                else
                {
                    if (enqueueCommand(&commandQueue, &command) != 0)
                    {
                        printf(COM2, "[engineerTask] Command buffer overflow!\n\r");
                        assert(0);
                    }
                }
                break;
            }

            case setReverse:
            {
                Reply(tid, 0, 0);

                Command command = {
                    .type = COMMAND_REVERSE,
                    .speed = speed; // this might be stale when command is actually executed (very unlikely)
                };

                if (commandWorkerTid > 0)
                {
                    Reply(commandWorkerTid, &command, sizeof(command));
                    commandWorkerTid = 0;
                }
                else
                {
                    if (enqueueCommand(&commandQueue, &command) != 0)
                    {
                        printf(COM2, "[engineerTask] Command buffer overflow!\n\r");
                        assert(0);
                    }
                }
                break;
            }

            // Command worker looking for work
            case commandWorker:
            {                
                if (isCommandQueueEmpty(&commandQueue))
                {
                    // case 1: no work; block it
                    commandWorkerTid = tid;
                }
                else
                {
                    // case 2: dequeue a job and reply the worker
                    Command command;
                    int ret = dequeueCommand(&commandQueue, &command);
                    assert(ret == 0);
                    Reply(tid, &command, sizeof(command));
                }
                break;
            }

            // Command worker: I've just issued reverse command!
            case commandWorkerSetReverse:
            {
                Reply(tid, 0, 0);

                if (state != Stop)
                {
                    printf(COM2, "[engineerTask] internal state error: train state should be Stop; current state: %d\n\r", state);
                    assert(0);
                }

                isForward = !isForward;
                state = (state == Init) ? Init : ReverseSetReverse;

                // reverse prevLandmark
                if (prevLandmark == 0)
                {
                    assert(0);
                }
                assert(prevLandmark != 0);
                track_node *nextLandmark = getNextLandmark(prevLandmark);
                assert(nextLandmark != 0);
                prevLandmark = nextLandmark->reverse;
                assert(prevLandmark != 0);
                break;
            }

            // Command worker: I've just issued a set speed command!
            case commandWorkerSetSpeed:
            {
                Reply(tid, 0, 0);
                speed = (char)(message.data.setSpeed.speed);
                if      (state == Init) break;
                else if (speed == 0) state = Stop;
                else    state = Running;
                break;
            }

            default:
            {
                printf(COM2, "Warning engineer Received garbage\r\n");
                assert(0);
                break;
            }
        }
    }
}

void initEngineer() {
    for(int i = 0; i < NUM_SENSORS; i++) {
        for(int j = 0; j < NUM_SENSORS; j++) {
            pairs[i][j] = 0;
        }
    }

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
    EngineerMessage message;
    message.type = reverse_direction;
    Send(engineerTaskId, &message, sizeof(EngineerMessage), 0, 0);
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

void engineerXMarksTheSpot(int nodeNumber) {
    assert(0 <= nodeNumber && nodeNumber <= 139);
    assert(engineerTaskId >= 0);
    EngineerMessage message;
    message.type = xMark;
    message.data.xMark.nodeNumber = nodeNumber;
    Send(engineerTaskId, &message, sizeof(EngineerMessage), 0, 0);
}

void engineerSpeedUpdate(int speed) {
    EngineerMessage message;
    message.type = setSpeed;
    message.data.setSpeed.speed = speed;
    assert(engineerTaskId >= 0);
    Send(engineerTaskId, &message, sizeof(EngineerMessage), 0, 0);
}
