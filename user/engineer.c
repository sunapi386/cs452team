#include <user/engineer.h>
#include <priority.h>
#include <debug.h> // uassert
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

track_node *getNextNode(track_node *current_landmark) {
    track_edge *next_edge = getNextEdge(current_landmark);
    return (next_edge == 0 ? 0 : next_edge->dest);
}

// returns positive distance between two nodes in micrometers
// or -1 if the landmark is not found
int distanceBetween(track_node *from, track_node *to)
{
    uassert(from != 0);
    uassert(to != 0);

    int totalDistance = 0;
    track_node *nextNode = from;
    for(int i = 0; i < TRACK_MAX / 2; i++)
    {
        // get next edge & node
        track_edge *nextEdge = getNextEdge(nextNode);
        nextNode = getNextNode(nextNode);
        if (nextEdge == 0 || nextNode == 0) break;

        totalDistance += nextEdge->dist;

        if (nextNode == to) return 1000 * totalDistance;
    }
    // unable to find the
    return -1;
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
    Command command;

    for (;;)
    {
        // Send to engineer to get new command
        message.type = commandWorkerRequest;
        int len = Send(pid, &message, sizeof(message), &command, sizeof(command));
        uassert(len == sizeof(Command));

executeCommand:
        switch(command.type)
        {
        case COMMAND_SET_SPEED:
            trainSetSpeed(command.trainNumber, command.trainSpeed);

            // Notify the engineer that the set speed command has been issued
            message.type = commandWorkerSpeedSet;
            message.data.setSpeed.speed = command.trainSpeed;
            Send(pid, &message, sizeof(message), 0, 0);
            break;
        case COMMAND_REVERSE:
            trainSetSpeed(command.trainNumber, 0);

            // Notify the engineer that the set speed command has been issued
            message.type = commandWorkerSpeedSet;
            message.data.setSpeed.speed = 0;
            Send(pid, &message, sizeof(message), 0, 0);

            // Wait until train comes to a stop and set reverse
            Delay(300); // TODO: scale this
            trainSetReverse(command.trainNumber);

            // Notify the engineer that the reverse command has been issued
            message.type = commandWorkerReverseSet;
            Send(pid, &message, sizeof(message), 0, 0);

            // Set the train back to original speed
            Delay(15);
            command.type = COMMAND_SET_SPEED;
            goto executeCommand;
            break;
        default:
            printf(COM2, "[commandWorker] Invalid command\n\r");
            uassert(0);
            break;
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
    int prevNodeTime = 0; // tick

    TrainState state = Init;
    track_node *prevLandmark = 0;
    SensorUpdate last_update = {0,0};
    int targetOffset = 0;
    track_node *targetNode = 0;
    int forwardStoppingDist[15] = {0, 100, 300, 600, 900, 1100, 2500, 4000, 5000, 6000, 6000, 1450000, 1250000, 948200,  948200};
    int backwardStoppingDist[15] = {0, 100, 300, 600, 900, 1100, 2500, 4000, 5000, 6000, 6000, 1450000, 1250000, 948200, 1000000};

    Command cmdBuf[32];
    CommandQueue commandQueue;
    initCommandQueue(&commandQueue, 32, &(cmdBuf[0]));

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
        uassert(len == sizeof(EngineerMessage));

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
                    uassert(Reply(tid, &uiMessage, sizeof(uiMessage)) != -1);
                    triggeredSensor = 0;
                    break;
                }

                uassert(prevLandmark != 0);

                // If we have a prevLandmark, compute the values needed
                // and update train display
                track_node *nextLandmark = getNextNode(prevLandmark);
                track_edge *nextEdge = getNextEdge(prevLandmark);
                if (nextEdge == 0 || nextLandmark == 0)
                {
                    printf(COM2, "[engineer:321] Warning: nextEdge/nextLandmark is NULL\n\r");
                    uiWorkerTid = tid;
                    break;
                }

                // compute some stuff
                int currentTime = Time();
                int delta = currentTime - prevNodeTime;
                int distSoFar = delta * velocity;
                int distToNextLandmark = nextEdge->dist * 1000 - distSoFar;

                // check for stopping at landmark
                if (targetNode != 0)
                {
                    int stoppingDistance = isForward ?
                        forwardStoppingDist[(int)speed] : backwardStoppingDist[(int)speed];

                    int dist = distanceBetween(prevLandmark, targetNode) - targetOffset - distSoFar;
                    if (dist <= stoppingDistance)
                    {
                        // issue stop command
                        Command cmd = {
                            .type = COMMAND_SET_SPEED,
                            .trainNumber = active_train,
                            .trainSpeed = 0,
                        };

                        // if worker is available, reply a command
                        if (commandWorkerTid > 0)
                        {
                            uassert(Reply(commandWorkerTid, &cmd, sizeof(cmd)) != -1);
                            commandWorkerTid = 0;
                        }
                        // else enqueue command
                        else
                        {
                            enqueueCommand(&commandQueue, &cmd);
                        }

                        // reset target
                        targetNode = 0;
                        targetOffset = 0;
                    }
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
                    uassert(Reply(tid, &uiMessage, sizeof(uiMessage)) != -1);
                }
                else if (distToNextLandmark <= 0)
                {
                    // We've "reached" the next non-sensor landmark
                    // Update previous landmark
                    prevLandmark = nextLandmark;
                    prevNodeTime = currentTime;

                    // update the next landmark
                    nextLandmark = getNextNode(prevLandmark);
                    track_edge *nextEdge = getNextEdge(prevLandmark);
                    if (nextLandmark == 0 || nextEdge == 0)
                    {
                        uiWorkerTid = tid;
                        printf(COM2, "[engineer:378] Warning: nextEdge/nextLandmark is NULL\n\r");
                        break;
                    }

                    // update the distance to next landmark
                    distToNextLandmark = nextEdge->dist * 1000;

                    // Output: next landmark, prev landmark, dist to next
                    uiMessage.type = UI_MESSAGE_LANDMARK;
                    uiMessage.nextLandmark = nextLandmark->name;
                    uiMessage.prevLandmark = prevLandmark->name;
                    uiMessage.distToNext = distToNextLandmark;
                    uassert(Reply(tid, &uiMessage, sizeof(uiMessage)) != -1);

                }
                else // (distanceToNextLandmark > 0)
                {
                    // We haven't reached the next landmark;
                    // Output: distance to next (already computed)
                    uiMessage.type = UI_MESSAGE_DIST;
                    uiMessage.distToNext = distToNextLandmark;
                    uassert(Reply(tid, &uiMessage, sizeof(uiMessage)) != -1);
                }
                break;
            }

            case updateSensor: {
                // unblock sensor
                uassert(Reply(tid, 0, 0) != -1);

                // Get sensor update data
                SensorUpdate sensor_update = {
                    message.data.updateSensor.sensor,
                    message.data.updateSensor.time
                };

                // Reject sensor updates with timestamp before creationTime
                if (sensor_update.time <= creationTime) continue;

                // Update some stuff
                prevLandmark = &g_track[indexFromSensorUpdate(&sensor_update)];
                prevNodeTime = sensor_update.time;

                track_node *nextLandmark = getNextNode(prevLandmark);
                track_edge *nextEdge = getNextEdge(prevLandmark);
                uassert(nextLandmark && nextEdge);

                // update the current velocity
                int timeSensors = sensor_update.time - last_update.time;
                int distSensors = distanceBetween(&g_track[indexFromSensorUpdate(&last_update)],
                                                  &g_track[indexFromSensorUpdate(&sensor_update)]);
                velocity = distSensors / timeSensors;

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

                creationTime = sensor_update.time;
                last_update.time = sensor_update.time;
                last_update.sensor = sensor_update.sensor;

                if (state == Init)
                {
                    state = speed ? Running : Stop;

                    // kick start UI refresh
                    uiMessage.type = UI_MESSAGE_INIT;
                    uassert(Reply(uiWorkerTid, &uiMessage, sizeof(uiMessage)) != -1);
                    uiWorkerTid = 0;
                }
                break;
            }

            case xMark:
            {
                uassert(Reply(tid, 0, 0) != -1);
                targetNode = &g_track[message.data.xMark.index];
                targetOffset = message.data.xMark.offset;
                break;
            }

            case setSpeed:
            {
                uassert(Reply(tid, 0, 0) != -1);

                Command command = {
                    .type = COMMAND_SET_SPEED,
                    .trainNumber = (char)(active_train),
                    .trainSpeed = (char)(message.data.setSpeed.speed)
                };

                if (commandWorkerTid > 0)
                {
                    uassert(Reply(commandWorkerTid, &command, sizeof(command)) != -1);
                    commandWorkerTid = 0;
                }
                else
                {
                    if (enqueueCommand(&commandQueue, &command) != 0)
                    {
                        printf(COM2, "[engineerTask] Command buffer overflow!\n\r");
                        uassert(0);
                    }
                }
                break;
            }

            case setReverse:
            {
                uassert(Reply(tid, 0, 0) != -1);

                Command command = {
                    .type = COMMAND_REVERSE,
                    .trainNumber = active_train,
                    .trainSpeed = speed
                };

                if (commandWorkerTid > 0)
                {
                    uassert(Reply(commandWorkerTid, &command, sizeof(command)) != -1);
                    commandWorkerTid = 0;
                }
                else
                {
                    if (enqueueCommand(&commandQueue, &command) != 0)
                    {
                        printf(COM2, "[engineerTask] Command buffer overflow!\n\r");
                        uassert(0);
                    }
                }
                break;
            }

            // Command worker looking for work
            case commandWorkerRequest:
            {
                if (isCommandQueueEmpty(&commandQueue))
                {
                    // case 1: no work; block it
                    printf(COM2, "[command worker request] command queue empty\n\r");
                    commandWorkerTid = tid;
                }
                else
                {
                    // case 2: dequeue a job and reply the worker
                    Command command;
                    int ret = dequeueCommand(&commandQueue, &command);
                    printf(COM2, "[command worker request] dequeued command: %d\n\r", command.type);
                    uassert(ret == 0);
                    uassert(Reply(tid, &command, sizeof(command)) != -1);
                }
                break;
            }

            // Command worker: I've just issued reverse command!
            case commandWorkerReverseSet:
            {
                uassert(Reply(tid, 0, 0) != -1);

                uassert(state == Stop);

                isForward = !isForward;
                state = (state == Init) ? Init : ReverseSetReverse;

                // reverse prevLandmark
                if (prevLandmark == 0)
                {
                    uassert(0);
                }
                uassert(prevLandmark != 0);
                track_node *nextLandmark = getNextNode(prevLandmark);
                uassert(nextLandmark != 0);
                prevLandmark = nextLandmark->reverse;
                uassert(prevLandmark != 0);
                break;
            }

            // Command worker: I've just issued a set speed command!
            case commandWorkerSpeedSet:
            {
                uassert(Reply(tid, 0, 0) != -1);
                speed = (char)(message.data.setSpeed.speed);
                if      (state == Init) break;
                else if (speed == 0) state = Stop;
                else    state = Running;
                break;
            }

            default:
            {
                printf(COM2, "Warning engineer Received garbage\r\n");
                uassert(0);
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
    uassert(engineerTaskId >= 0);
}

void engineerPleaseManThisTrain(int train_number, int speed) {
    uassert(1 <= train_number && train_number <= 80 && 0 <= speed && speed <= 14);
    active_train = train_number;
}

void engineerReverse() {
    EngineerMessage message;
    message.type = setReverse;
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

void engineerXMarksTheSpot(int index, int offset) {
    uassert(0 <= index && index <= 139);
    uassert(engineerTaskId >= 0);
    EngineerMessage message;
    message.type = xMark;
    message.data.xMark.index = index;
    message.data.xMark.offset = offset * 1000; // convert mm to um
    Send(engineerTaskId, &message, sizeof(EngineerMessage), 0, 0);
}

void engineerSpeedUpdate(int speed) {
    EngineerMessage message;
    message.type = setSpeed;
    message.data.setSpeed.speed = speed;
    uassert(engineerTaskId >= 0);
    Send(engineerTaskId, &message, sizeof(EngineerMessage), 0, 0);
}
