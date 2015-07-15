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
#define NUM_SPEEDS          15
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
    const char *nextNode;
    const char *prevNode;
} UIMessage;

typedef enum {
    Init = 0,           // initial state
    Stop,               // stop command issued
    Running,            // running
    ReverseSetReverse,  // reverse stage 2: reverse issued
} TrainState;

static int active_train = -1; // static for now, until controller is implemented
static int timeDeltas[NUM_SPEEDS][NUM_SENSORS][NUM_SENSORS];
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

track_edge *getNextEdge(track_node *node)
{
    if(node->type == NODE_BRANCH)
    {
        return &node->edge[turnoutIsCurved(node->num)];
    }
    else if (node->type == NODE_EXIT)
    {
        return 0;
    }
    return &node->edge[DIR_AHEAD];
}

track_node *getNextNode(track_node *currentNode) {
    track_edge *next_edge = getNextEdge(currentNode);
    return (next_edge == 0 ? 0 : next_edge->dest);
}

track_node *getNextSensor(track_node *node)
{
    uassert(node != 0);
    for (;;)
    {
        node = getNextNode(node);
        if (node == 0 || node->type == NODE_SENSOR) return node;
    }
    return 0;
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

void updateScreenNewLandmark(const char *nextNode,
                             const char *prevNode,
                             int distToNext)
{
    printf(COM2, VT_CURSOR_SAVE
                 "\033[1;100H%s    "
                 "\033[2;100H%s    "
                 "\033[3;100H%d.%d cm     "
                 VT_CURSOR_RESTORE,
                 nextNode,
                 prevNode,
                 distToNext / 10000,
                 abs(distToNext) % 10000);
}

void updateScreenNewSensor(const char *nextNode,
                           const char *prevNode,
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
                 nextNode,
                 prevNode,
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
            updateScreenNewLandmark(uiMessage.nextNode,
                                    uiMessage.prevNode,
                                    uiMessage.distToNext);
            break;
        case UI_MESSAGE_SENSOR:
            updateScreenNewSensor(uiMessage.nextNode,
                                  uiMessage.prevNode,
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

    TrainState state = Init;

    bool isForward = true;
    char speed = 0;   // 0-14
    int triggeredSensor = 0;  // {0|1}

    int distSoFar = 0;
    int didReverse = 0;
    track_node *prevNode = 0;
    int prevNodeTime = 0; // ticks

    /*
        linear acceleration
            a = (vf * vf - vi * vi) / 2 * s

        vf: final speed       (0)
        vi: initial speed     (calculated)
        s:  stopping distance (from table)
    */
    int vf = 0;
    int vi = 0;

    int sensorDist = 0;
    track_node *prevSensor = 0;
    track_node *nextSensor = 0;

    SensorUpdate last_update = {0,0};
    int targetOffset = 0;
    track_node *targetNode = 0;
    int stoppingDistance[15] =
        {0, 100, 300, 600, 900, 1100, 2500, 4000, 5000, 6000, 6000, 1450000, 1250000, 948200,  948200};

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

                uassert(prevNode != 0 && prevSensor != 0 && nextSensor != 0);

                // If we have a prevNode, compute the values needed
                // and update train display
                track_node *nextNode = getNextNode(prevNode);
                track_edge *nextEdge = getNextEdge(prevNode);
                if (nextEdge == 0 || nextNode == 0)
                {
                    printf(COM2, "[engineer:321] Warning: nextEdge/nextNode is NULL\n\r");
                    uiWorkerTid = tid;
                    break;
                }

                /* distance calculations */
                int currentTime = Time();
                int distToNextNode;
                if (didReverse)
                {
                    distToNextNode = distSoFar;
                    didReverse = 0;
                }
                else
                {
                    int delta = currentTime - prevNodeTime;
                    int velocity = sensorDist / timeDeltas[(int)speed][prevSensor->idx][nextSensor->idx];
                    distSoFar = delta * velocity;
                    distToNextNode = nextEdge->dist * 1000 - distSoFar;
                }

                // check for stopping at landmark
                if (targetNode != 0)
                {
                    int pickUpOffset = isForward ? 0 : 15 * 1000;
                    int dist = distanceBetween(prevNode, targetNode) - pickUpOffset - targetOffset - distSoFar;
                    if (dist <= stoppingDistance[(int)speed])
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

                if (nextNode->type == NODE_SENSOR)
                {
                    // output the distance to next landmark
                    uiMessage.type = UI_MESSAGE_DIST;
                    uiMessage.distToNext = distToNextNode;
                    uassert(Reply(tid, &uiMessage, sizeof(uiMessage)) != -1);
                }
                else if (distToNextNode <= 0)
                {
                    // We've "reached" the next non-sensor landmark
                    // Update previous landmark
                    prevNode = nextNode;
                    prevNodeTime = currentTime;

                    // update the next landmark
                    nextNode = getNextNode(prevNode);
                    track_edge *nextEdge = getNextEdge(prevNode);
                    if (nextNode == 0 || nextEdge == 0)
                    {
                        uiWorkerTid = tid;
                        printf(COM2, "[engineer:378] Warning: nextEdge/nextNode is NULL\n\r");
                        break;
                    }

                    // update the distance to next landmark
                    distToNextNode = nextEdge->dist * 1000;

                    // Output: next landmark, prev landmark, dist to next
                    uiMessage.type = UI_MESSAGE_LANDMARK;
                    uiMessage.nextNode = nextNode->name;
                    uiMessage.prevNode = prevNode->name;
                    uiMessage.distToNext = distToNextNode;
                    uassert(Reply(tid, &uiMessage, sizeof(uiMessage)) != -1);

                }
                else // (distanceToNextNode > 0)
                {
                    // We haven't reached the next landmark;
                    // Output: distance to next (already computed)
                    uiMessage.type = UI_MESSAGE_DIST;
                    uiMessage.distToNext = distToNextNode;
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
                prevNode = &g_track[indexFromSensorUpdate(&sensor_update)];
                prevNodeTime = sensor_update.time;
                prevSensor = prevNode;

                nextSensor = getNextSensor(prevSensor);
                sensorDist = distanceBetween(prevSensor, nextSensor);

                track_node *nextNode = getNextNode(prevNode);
                track_edge *nextEdge = getNextEdge(prevNode);
                uassert(nextNode && nextEdge);

                // update constant velocity calibration data
                int last_index = indexFromSensorUpdate(&last_update);
                int index = indexFromSensorUpdate(&sensor_update);

                int expectedTime = timeDeltas[(int)speed][last_index][index];
                int actualTime = sensor_update.time - last_update.time;
                int error = abs(expectedTime - actualTime);

                if(last_index >= 0)
                {
                    // apply learning
                    int past_difference = timeDeltas[(int)speed][last_index][index];
                    int new_difference = sensor_update.time - last_update.time;
                    timeDeltas[(int)speed][last_index][index] =  (new_difference * ALPHA
                        + past_difference * (100 - ALPHA)) / 100;

                    // prepare output
                    int distanceToNextNode = nextEdge->dist * 1000;

                    uiMessage.type = UI_MESSAGE_SENSOR;
                    uiMessage.nextNode = nextNode->name;
                    uiMessage.prevNode = prevNode->name;
                    uiMessage.distToNext = distanceToNextNode;
                    uiMessage.prevSensor = prevNode->name;
                    uiMessage.expectedTime = expectedTime;
                    uiMessage.actualTime = actualTime;
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
                    commandWorkerTid = tid;
                }
                else
                {
                    // case 2: dequeue a job and reply the worker
                    Command command;
                    int ret = dequeueCommand(&commandQueue, &command);
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

                // reverse prevNode
                if (prevNode == 0)
                {
                    uassert(0);
                }
                uassert(prevNode != 0);
                track_node *oldPrevNode = prevNode;
                track_node *temp = getNextNode(prevNode);
                uassert(temp != 0);
                prevNode = temp->reverse;
                uassert(prevNode != 0);

                // swap prev & next sensor
                temp = prevSensor;
                prevSensor = nextSensor->reverse;
                nextSensor = temp->reverse;

                printf(COM2, "Reverse: updating prevNode; old: %s new: %s\n\r prevSensor: %s nextSensor: %s\n\r",
                    oldPrevNode->name,
                    prevNode->name,
                    prevSensor->name,
                    nextSensor->name);

                didReverse = 1;

                break;
            }

            // Command worker: I've just issued a set speed command!
            case commandWorkerSpeedSet:
            {
                uassert(Reply(tid, 0, 0) != -1);
                speed = (char)(message.data.setSpeed.speed);
                if      (state == Init) break;
                else if (speed == 0)
                {
                    state = Stop;
                }
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
    for(int i = 0; i < NUM_SPEEDS; i++) {
        for(int j = 0; j < NUM_SENSORS; j++) {
            for (int k = 0; k < NUM_SENSORS; k++) {
                int val = 50;
                if (i == 0) val = 0;

                timeDeltas[i][j][k] = val;
            }
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
