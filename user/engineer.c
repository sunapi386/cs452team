#include <user/engineer.h>
#include <priority.h>
#include <debug.h>
#include <user/syscall.h>
#include <utils.h>
#include <user/train.h>
#include <user/sensor.h>
#include <user/pathfinding.h>
#include <user/track_data.h>
#include <user/turnout.h>
#include <user/vt100.h>

#define ALPHA                85
#define NUM_SENSORS          (5 * 16)
#define NUM_TRAINS           80
#define NUM_SPEEDS           15
#define UI_REFRESH_INTERVAL  5

#define LOC_MESSAGE_INIT     20
#define LOC_MESSAGE_DIST     21
#define LOC_MESSAGE_LANDMARK 22
#define LOC_MESSAGE_SENSOR   23

typedef struct LocationMessage {
    int type;
    int error;
    int distToNext;
    int expectedTime;
    int actualTime;
    const char *prevSensor;
    const char *nextNode;
    const char *prevNode;
} LocationMessage;

typedef enum {
    Init = 0,           // initial state
    Stop,               // stop command issued
    Running,            // running
    Reversing,  // reverse stage 2: reverse issued
    Decelerating
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
static void sensorCourier() {
    int engineer = MyParentTid();
    int sensor = WhoIs("sensorServer");

    SensorRequest sensorReq;  // courier <-> sensorServer
    sensorReq.type = MESSAGE_SENSOR_COURIER;
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

static inline void initializeScreen()
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

static inline void updateScreenDistToNext(int distToNext)
{
    // convert from um to cm
    printf(COM2, VT_CURSOR_SAVE
                 "\033[3;100H%d.%d cm     "
                 VT_CURSOR_RESTORE,
                 distToNext / 10000,
                 abs(distToNext) % 10000);
}

static inline void updateScreenNewLandmark(const char *nextNode,
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

static inline void updateScreenNewSensor(const char *nextNode,
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

void locationWorker()
{
    int pid = MyParentTid();

    LocationMessage locationMessage;
    EngineerMessage engMessage;
    engMessage.type = updateLocation;

    for (;;)
    {
        // receive data to display
        int len = Send(pid, &engMessage, sizeof(engMessage), &locationMessage, sizeof(locationMessage));
        assert(len == sizeof(locationMessage));

        // update display
        switch (locationMessage.type)
        {
        case LOC_MESSAGE_DIST:
            updateScreenDistToNext(locationMessage.distToNext);
            break;
        case LOC_MESSAGE_LANDMARK:
            updateScreenNewLandmark(locationMessage.nextNode,
                                    locationMessage.prevNode,
                                    locationMessage.distToNext);
            break;
        case LOC_MESSAGE_SENSOR:
            updateScreenNewSensor(locationMessage.nextNode,
                                  locationMessage.prevNode,
                                  locationMessage.distToNext,
                                  locationMessage.prevSensor,
                                  locationMessage.expectedTime,
                                  locationMessage.actualTime,
                                  locationMessage.error);

            break;
        case LOC_MESSAGE_INIT:
            initializeScreen();
            break;
        default:
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
    int locationWorkerTid = 0;
    int commandWorkerTid = 0;

    TrainState state = Init;

    bool isForward = true;
    char speed = 0;
    char prevSpeed = 0;
    int triggeredSensor = 0;

    int velocity = 0;
    int distSoFar = 0;
    int deceleration = 0;
    int prevNodeTime = 0;
    int lastLocationUpdateTime = 0;
    track_node *prevNode = 0;

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

    LocationMessage locationMessage;
    EngineerMessage message;

    // Create child tasks
    Create(PRIORITY_SENSOR_COURIER, sensorCourier);
    Create(PRIORITY_SENSOR_COURIER, commandWorker);
    locationWorkerTid = Create(PRIORITY_SENSOR_COURIER, locationWorker);

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
                    Reply(tid, &locationMessage, sizeof(locationMessage));
                    triggeredSensor = 0;
                    break;
                }

                uassert(prevNode != 0 && prevSensor != 0 && nextSensor != 0);

                // If we have a prevNode, compute the values needed
                // and update train display
                track_node *nextNode = getNextNode(prevNode);
                track_edge *nextEdge = getNextEdge(prevNode);
                uassert(nextNode && nextEdge);

                /* distance calculations */
                int currentTime = Time();

                if (state == Decelerating)
                {
                    if (deceleration == 0)
                    {
                        // calculate deceleration for the current speed:
                        // a = (vf * vf - vi * vi) / 2 * s
                        deceleration = velocity * velocity / (2 * stoppingDistance[(int)prevSpeed]);
                    }

                    velocity -= deceleration * (currentTime - lastLocationUpdateTime);
                    if (velocity <= 0)
                    {
                        velocity = 0;
                        deceleration = 0;
                        state = Stop;
                    }
                }
                else if (state == Stop)
                {
                    velocity = 0;
                }
                else
                {
                    velocity = sensorDist / timeDeltas[(int)speed][prevSensor->idx][nextSensor->idx];
                }

                // velocity: the estimated velocity in the current track segment
                // lasttime: the last time distSoFar is incremented
                distSoFar += (currentTime - lastLocationUpdateTime) * velocity;
                lastLocationUpdateTime = currentTime;
                int distToNextNode = nextEdge->dist * 1000 - distSoFar;

                // check for stopping at landmark
                if (targetNode != 0)
                {
                    int pickUpOffset = isForward ? 0 : 15 * 1000;
                    int dist = distanceBetween(prevNode, targetNode) - pickUpOffset - targetOffset - distSoFar;

                    int stoppingDist = stoppingDistance[(int)speed];
                    if (dist >= 0 && stoppingDist > 0 && dist <= stoppingDist)
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
                            Reply(commandWorkerTid, &cmd, sizeof(cmd));
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

                if (distToNextNode <= 0)
                {
                    // We've "reached" the next non-sensor landmark
                    // Update previous landmark
                    prevNode = nextNode;
                    prevNodeTime = currentTime;

                    // get the next landmark
                    nextNode = getNextNode(prevNode);
                    track_edge *nextEdge = getNextEdge(prevNode);
                    uassert(nextNode != 0 && nextEdge != 0);

                    // resetting distSoFar: non-sensor node reached
                    distSoFar = 0;

                    // update the distance to next landmark
                    distToNextNode = nextEdge->dist * 1000;

                    // Output: next landmark, prev landmark, dist to next
                    locationMessage.type = LOC_MESSAGE_LANDMARK;
                    locationMessage.nextNode = nextNode->name;
                    locationMessage.prevNode = prevNode->name;
                    locationMessage.distToNext = distToNextNode;
                    Reply(tid, &locationMessage, sizeof(locationMessage));
                }
                else if (state == Reversing)
                {
                    // swapping distToNextNode with distSoFar
                    int temp = distSoFar;
                    distSoFar = distToNextNode;
                    distToNextNode = temp;

                    // get the next landmark
                    nextNode = getNextNode(prevNode);

                    // output
                    locationMessage.type = LOC_MESSAGE_LANDMARK;
                    locationMessage.nextNode = nextNode->name;
                    locationMessage.prevNode = prevNode->name;
                    locationMessage.distToNext = distToNextNode;
                    Reply(tid, &locationMessage, sizeof(locationMessage));

                    state = (speed == 0) ? Stop : Running;
                }
                else // distToNextNode > 0
                {
                    // We haven't reached the next landmark;
                    // Output: distance to next (already computed)
                    locationMessage.type = LOC_MESSAGE_DIST;
                    locationMessage.distToNext = distToNextNode;
                    Reply(tid, &locationMessage, sizeof(locationMessage));
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
                prevNode = &g_track[indexFromSensorUpdate(&sensor_update)];
                prevNodeTime = sensor_update.time;
                prevSensor = prevNode;

                nextSensor = getNextSensor(prevSensor);
                sensorDist = distanceBetween(prevSensor, nextSensor);
                uassert(sensorDist >= 0);

                track_node *nextNode = getNextNode(prevNode);
                track_edge *nextEdge = getNextEdge(prevNode);
                uassert(nextNode && nextEdge);

                // resetting distSoFar: non-sensor node reached
                distSoFar = 0;

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

                    locationMessage.type = LOC_MESSAGE_SENSOR;
                    locationMessage.nextNode = nextNode->name;
                    locationMessage.prevNode = prevNode->name;
                    locationMessage.distToNext = distanceToNextNode;
                    locationMessage.prevSensor = prevNode->name;
                    locationMessage.expectedTime = expectedTime;
                    locationMessage.actualTime = actualTime;
                    locationMessage.error = error;
                    triggeredSensor = 1;
                }

                creationTime = sensor_update.time;
                last_update.time = sensor_update.time;
                last_update.sensor = sensor_update.sensor;

                if (state == Init)
                {
                    state = speed ? Running : Stop;

                    // kick start UI refresh
                    locationMessage.type = LOC_MESSAGE_INIT;
                    Reply(locationWorkerTid, &locationMessage, sizeof(locationMessage));
                    locationWorkerTid = 0;
                }
                break;
            }

            case xMark:
            {
                Reply(tid, 0, 0);
                targetNode = &g_track[message.data.xMark.index];
                targetOffset = message.data.xMark.offset;
                break;
            }

            case setSpeed:
            {
                Reply(tid, 0, 0);

                Command command = {
                    .type = COMMAND_SET_SPEED,
                    .trainNumber = (char)(active_train),
                    .trainSpeed = (char)(message.data.setSpeed.speed)
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
                        uassert(0);
                    }
                }
                break;
            }

            case setReverse:
            {
                Reply(tid, 0, 0);

                Command command = {
                    .type = COMMAND_REVERSE,
                    .trainNumber = active_train,
                    .trainSpeed = speed
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
                    Reply(tid, &command, sizeof(command));
                }
                break;
            }

            // Command worker: I've just issued reverse command!
            case commandWorkerReverseSet:
            {
                Reply(tid, 0, 0);

                isForward = !isForward;
                state = (state == Init) ? Init : Reversing;

                // reverse prevNode
                uassert(prevNode != 0);
                track_node *temp = getNextNode(prevNode);
                uassert(temp != 0);
                prevNode = temp->reverse;
                uassert(prevNode != 0);

                // swap prev & next sensor
                temp = prevSensor;
                prevSensor = nextSensor->reverse;
                nextSensor = temp->reverse;
                break;
            }

            // Command worker: I've just issued a set speed command!
            case commandWorkerSpeedSet:
            {
                Reply(tid, 0, 0);
                prevSpeed = speed;
                speed = (char)(message.data.setSpeed.speed);
                printf(COM2, "Speed set: %d, currentState: %d\n\r", speed, state);
                if      (state == Init || state == Reversing) break;
                else if (speed == 0) state = Decelerating;
                else    state = Running;
                break;
            }

            default:
            {
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
