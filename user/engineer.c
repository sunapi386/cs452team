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
#include <user/engineer_ui.h>
#include <user/trackserver.h>

#define ALPHA                85
#define NUM_SENSORS          (5 * 16)
#define NUM_TRAINS           80
#define NUM_SPEEDS           15
#define UI_REFRESH_INTERVAL  5

#define LOC_MESSAGE_INIT     20
#define LOC_MESSAGE_DIST     21
#define LOC_MESSAGE_LANDMARK 22
#define LOC_MESSAGE_SENSOR   23

#define PICK_UP_OFFSET (105 * 1000)

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
    Init = 0,        // initial state
    Ready,           // setup complete
    Stopped,         // stop command issued
    Running,         // running
    Reversing,       // reverse issued
    Decelerating,    // comming to a stop
    Accelerating
} TrainState;

extern track_node g_track[TRACK_MAX];

static inline int abs(int num) {
    return (num < 0 ? -1 * num : num);
}

/**
    Sensor Courier

    First submit the inital claim to the sensor server, gets unblocked
    immedately.
    Then block on the engineer. Whenever there is a new claim, or the existing
    claim needs to be invalidated, the engineer unblocks the sensor courier by
    replying to it with the message that it needs to send to sensor server.

    The sensor server always immedietly unblocks the sensor courier.

    The data carried from sensor courier to the sensor server must include the
    tid of the engieer, which is basically the parent tid of the sensor courier.


    Q: Can a sensor claim fail?

    A: Sensor claim won't fail as the track server (from whom the engineer asks
    for reservations) runs at a higher priority. So it will never be the case
    that another engineer is in mid-call to getting reservations and you
    interrupt them. So it is atomic.
*/

static void sensorCourier(int initialSensorIndex)
{
    uassert(initialSensorIndex >= 0 && initialSensorIndex < 80);
    printf(COM2, "Sensor courier started; inital sensor index: %d\n\r",
        initialSensorIndex);

    int engineerTid = MyParentTid();
    int sensorTid = WhoIs("sensorServer");

    // claim the first sensor
    SensorRequest sensorReq;
    sensorReq.type = initialClaim;
    sensorReq.data.initialClaim.engineerTid = engineerTid;
    sensorReq.data.initialClaim.index = initialSensorIndex;

    // message to block on engineer: give me next sensor claim!
    EngineerMessage engineerReq; // courier <-> engineer
    engineerReq.type = sensorCourierRequest;

    for (;;)
    {
        // claim sensor at the sensor server
        Send(sensorTid, &sensorReq, sizeof(sensorReq), 0, 0);

        // block on engineer until replied with the next sensor claim
        Send(engineerTid,
            &engineerReq,
            sizeof(engineerReq),
            &(sensorReq.data.claimSensor),
            sizeof(sensorReq.data.claimSensor));

        // reset sensor request type
        sensorReq.type = claimSensor;
    }
}

/*
    Location Worker
*/

void locationWorker(int numEngineer)
{
    int pid = MyParentTid();

    LocationMessage locationMessage;
    EngineerMessage engMessage;
    engMessage.type = updateLocation;

    for (;;)
    {
        // receive data to display
        int len = Send(pid, &engMessage, sizeof(engMessage),
                            &locationMessage, sizeof(locationMessage));
        assert(len == sizeof(locationMessage));

        // update display
        switch (locationMessage.type)
        {
        case LOC_MESSAGE_DIST:
            updateScreenDistToNext(numEngineer, locationMessage.distToNext);
            break;
        case LOC_MESSAGE_LANDMARK:
            updateScreenNewLandmark(numEngineer,
                                    locationMessage.nextNode,
                                    locationMessage.prevNode,
                                    locationMessage.distToNext);
            break;
        case LOC_MESSAGE_SENSOR:
            updateScreenNewSensor(numEngineer,
                                  locationMessage.nextNode,
                                  locationMessage.prevNode,
                                  locationMessage.distToNext,
                                  locationMessage.prevSensor,
                                  locationMessage.expectedTime,
                                  locationMessage.actualTime,
                                  locationMessage.error);

            break;
        case LOC_MESSAGE_INIT:
            initializeScreen(numEngineer);
            break;
        default:
            assert(0);
            break;
        }

        // delay until the next update
        Delay(UI_REFRESH_INTERVAL);
    }
}

/*
    Command Worker
*/

void commandWorker()
{
    int pid = MyParentTid();

    EngineerMessage message;
    Command command;

    for (;;)
    {
        // Send to engineer to get new command
        message.type = commandWorkerRequest;
        int len = Send(pid, &message, sizeof(message),
                            &command, sizeof(command));
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

/*
    Engineer
*/

void initializeEngineer(int numEngineer, int *trainNumber, int *locationWorkerTid)
{
    int tid = 0;
    EngineerMessage message;
    /*
        Stage 1: Initalize trainNumber and initialSensorIndex
    */
    int ret = Receive(&tid, &message, sizeof(EngineerMessage));
    uassert(ret == sizeof(EngineerMessage) && message.type == initialize);

    // unblock parser
    Reply(tid, 0, 0);

    // receive inital values: train number and inital sensor index
    *trainNumber = message.data.initialize.trainNumber;
    int initialSensorIndex = message.data.initialize.sensorIndex;

    /*
        Stage 2: Create child tasks
    */
    ret = Spawn(PRIORITY_SENSOR_COURIER, sensorCourier, (void *)initialSensorIndex);
    uassert(ret > 0);

    ret = Create(PRIORITY_COMMAND_WORKER, commandWorker);
    uassert(ret > 0);

    *locationWorkerTid = Spawn(PRIORITY_LOCATION_WORKER,
                                locationWorker, (void *)numEngineer);
    uassert(*locationWorkerTid > 0);
}

void issueSpeedCommand(int trainNumber,
                       int speed,
                       int *commandWorkerTid,
                       CommandQueue *commandQueue)
{
    // issue stop command
    Command cmd = {
        .type = COMMAND_SET_SPEED,
        .trainNumber = trainNumber,
        .trainSpeed = speed,
    };

    // if worker is available, reply a command
    if (*commandWorkerTid > 0)
    {
        Reply(*commandWorkerTid, &cmd, sizeof(cmd));
        *commandWorkerTid = 0;
    }
    // else enqueue command
    else
    {
        enqueueCommand(commandQueue, &cmd);
    }
}


/*
    Given a node and a distance, adding all the nodes within that distance, from the
    initial node, into reserveNodes array
*/
int getReserveNodes(track_node *from, int reserveDist, track_node *reserveNodes[])
{
    uassert(from != 0);

    int dist = 0;
    track_node *nextNode = from;
    reserveNodes[0] = nextNode;
    for (int i = 1; i < MAX_RESERVE_SIZE; i++)
    {
        // return when we are above/at reserveDist
        if (dist >= reserveDist || nextNode->type == NODE_EXIT) return i;

        // get the next node and next edge
        track_edge *nextEdge = getNextEdge(nextNode);
        nextNode = getNextNode(nextNode);

        // add the next
        reserveNodes[i] = nextNode;

        // increment the distance
        dist += nextEdge->dist;
    }
    uassert(0);
    return -1;
}

void engineerServer(int numEngineer)
{
    int tid = 0;
    int locationWorkerTid = 0;
    int commandWorkerTid = 0;
    int sensorCourierTid = 0;

    int trainNumber = 0;
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
    int hasOutstandingClaim = 0;
    SensorClaim outstandingClaim;

    SensorUpdate last_update = {0,0};
    int targetOffset = 0;
    track_node *targetNode = 0;
    int stoppingDistance[15] =
        {0, 100, 300, 600, 900, 1100, 2500, 4000, 5000, 6000, 6000,
            1450000, 1250000, 948200,  948200};

    Command cmdBuf[32];
    CommandQueue commandQueue;
    initCommandQueue(&commandQueue, 32, &(cmdBuf[0]));

    LocationMessage locationMessage;
    EngineerMessage message;

    // initialize timeDeltas table
    int timeDeltas[NUM_SPEEDS][NUM_SENSORS];
    for (int i = 0; i < NUM_SPEEDS; i++)
    {
        for (int j = 0; j < NUM_SENSORS; j++)
        {
            if (i == 0) timeDeltas[i][j] = 0;
            else        timeDeltas[i][j] = 50;
        }
    }

    initializeEngineer(numEngineer, &trainNumber, &locationWorkerTid);
    state = Ready;

    // All sensor reports with timestamp before this time are invalid
    int creationTime = Time();
    /**
    Ebook for the go command
    */
    Ebook ebook;

    for(;;)
    {
        int len = Receive(&tid, &message, sizeof(EngineerMessage));
        uassert(len == sizeof(EngineerMessage));

        switch(message.type)
        {
        case updateLocation:
        {
            if (state == Ready)
            {
                // Just starting, block until we reach the first sensor
                break;
            }

            if (triggeredSensor)
            {
                // Sensor was just hit, reply directly to update last sensor
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
                    deceleration = velocity *
                    velocity / (2 * stoppingDistance[(int)prevSpeed]);
                }

                velocity -= deceleration *
                            (currentTime - lastLocationUpdateTime);
                if (velocity <= 0)
                {
                    velocity = 0;
                    deceleration = 0;
                    state = Stopped;
                }
                distSoFar += (currentTime - lastLocationUpdateTime) *
                            velocity;
            }
            else if (state != Stopped)
            {
                velocity = sensorDist /
                            timeDeltas[(int)speed][nextSensor->idx];
                distSoFar = (currentTime - prevNodeTime) * velocity;
            }

            // velocity: the estimated velocity in the current track segment
            // lasttime: the last time distSoFar is incremented
            //distSoFar += (currentTime - lastLocationUpdateTime) * velocity;
            lastLocationUpdateTime = currentTime;
            int distToNextNode = nextEdge->dist * 1000 - distSoFar;

            // check for stopping at landmark
            if (targetNode != 0)
            {
                int pickUpOffset = isForward ? 0 : PICK_UP_OFFSET;
                int dist = distanceBetween(prevNode, targetNode) -
                            pickUpOffset - targetOffset - distSoFar;

                int stoppingDist = stoppingDistance[(int)speed];
                if (dist >= 0 && stoppingDist > 0 && dist <= stoppingDist)
                {
                    // issue stop command
                    Command cmd = {
                        .type = COMMAND_SET_SPEED,
                        .trainNumber = trainNumber,
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

            // from nextNode, to stoppingDist, try to reserve all the nodes in between,
            // starting from the closest node

            // if success, keep going
            track_node *reserveNodes[MAX_RESERVE_SIZE];
            track_node *releaseNodes[MAX_RELEASE_SIZE];
            int numReserve = getReserveNodes(nextNode, stoppingDistance[(int)speed], reserveNodes);
            if (numReserve > 0)
            {
                // reserve these mother fuckers
                int success ;//= reserve(node, numReserve);
                if (!success)
                {
                    // reservation failed. stop the train
                    issueSpeedCommand(trainNumber, 0, &commandWorkerTid, &commandQueue);
                }
            }
            else
            {
                // exit seen. issue stop command now
            }

            if (distToNextNode <= 0 && nextNode->type != NODE_SENSOR)
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

                state = (speed == 0) ? Stopped : Accelerating;
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
            uassert(state != Init);

            // unblock engineer courier
            Reply(tid, 0, 0);

            // Get sensor update data
            SensorUpdate sensor_update = {
                message.data.updateSensor.sensor,
                message.data.updateSensor.time
            };

            // Reject sensor updates with timestamp before creationTime
            if (sensor_update.time <= creationTime) continue;

            // Update some stuff
            prevNode = &g_track[sensor_update.sensor];
            prevNodeTime = sensor_update.time;
            prevSensor = prevNode;

            nextSensor = getNextSensor(prevSensor);
            sensorDist = distanceBetween(prevSensor, nextSensor);
            uassert(sensorDist >= 0);

            track_node *nextNode = getNextNode(prevNode);
            track_edge *nextEdge = getNextEdge(prevNode);
            uassert(nextNode && nextEdge);

            // resetting distSoFar: non-sensor node reached
            distSoFar = isForward ? 0 : PICK_UP_OFFSET;

            // update constant velocity calibration data
            int last_index = last_update.sensor;
            int index = sensor_update.sensor;

            int expectedTime = timeDeltas[(int)speed][index];
            int actualTime = sensor_update.time - last_update.time;
            int error = abs(expectedTime - actualTime);

            if(last_index >= 0)
            {
                // apply learning
                if (state == Running)
                {
                    int past_difference = timeDeltas[(int)speed][index];
                    int new_difference = sensor_update.time - last_update.time;
                    timeDeltas[(int)speed][index] =  (new_difference * ALPHA
                        + past_difference * (100 - ALPHA)) / 100;
                }

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

            if (state == Ready)
            {
                state = speed ? Accelerating : Stopped;

                // kick start UI refresh
                locationMessage.type = LOC_MESSAGE_INIT;
                Reply(locationWorkerTid, &locationMessage, sizeof(locationMessage));
                locationWorkerTid = 0;
            }
            else if (state == Accelerating)
            {
                // Assume after hitting sensor, train reaches constant velocity.
                // therefore transition to Running
                state = Running;
            }

            if (sensorCourierTid)
            {
                // compute the next claims
                SensorClaim claim;
                int ret = getNextClaims(prevSensor, &claim);
                uassert(ret == 0);

                Reply(sensorCourierTid, &claim, sizeof(claim));

                sensorCourierTid = 0;
            }
            else
            {
                uassert(hasOutstandingClaim == 0);

                // sensor courier is not back; we would need to store it in some variable,
                // and when the sensor courier request comes in, it will simply take that
                // claim instead of waiting to be unblocked
                int ret = getNextClaims(prevSensor, &outstandingClaim);
                uassert(ret == 0);
                hasOutstandingClaim = 1;
            }
            break;
        }

        // sensor courier: hey, give me the next claim!
        case sensorCourierRequest:
        {
            if (hasOutstandingClaim)
            {
                Reply(tid, &outstandingClaim, sizeof(outstandingClaim));

                hasOutstandingClaim = 0;

            }
            else
            {
                // if there is no outstanding claim, then block
                sensorCourierTid = tid;
            }
            break;
        }

        case xMark:
        {
            uassert(state != Init);
            Reply(tid, 0, 0);
            targetNode = &g_track[message.data.xMark.index];
            targetOffset = message.data.xMark.offset;
            break;
        }

        case setSpeed:
        {
            uassert(state != Init);
            Reply(tid, 0, 0);

            Command command = {
                .type = COMMAND_SET_SPEED,
                .trainNumber = (char)(trainNumber),
                .trainSpeed = (char)(message.data.setSpeed.speed)
            };

            if (commandWorkerTid > 0)
            {
                Reply(commandWorkerTid, &command, sizeof(command));
                commandWorkerTid = 0;
            }
            else if (enqueueCommand(&commandQueue, &command) != 0)
            {
                printf(COM2, "[engineerTaskengineerServer] Command buffer overflow!\n\r");
                uassert(0);
            }
            break;
        }

        case setReverse:
        {
            uassert(state != Init);
            Reply(tid, 0, 0);

            Command command = {
                .type = COMMAND_REVERSE,
                .trainNumber = trainNumber,
                .trainSpeed = speed
            };

            if (commandWorkerTid > 0)
            {
                Reply(commandWorkerTid, &command, sizeof(command));
                commandWorkerTid = 0;
            }
            else if (enqueueCommand(&commandQueue, &command) != 0)
            {
                printf(COM2, "[engineerServer] Command buffer overflow!\n\r");
                uassert(0);
            }
            break;
        }

        // Command worker looking for work
        case commandWorkerRequest:
        {
            uassert(state != Init);
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

            // update the claimed sensor to the new ones
            if (hasOutstandingClaim)
            {
                // if there is an outstanding claim, change it
                int ret = getNextClaims(prevSensor, &outstandingClaim);
                uassert(ret == 0);
            }
            else if (sensorCourierTid)
            {
                // unblock sensor courier with a new claim
                SensorClaim claim;
                int ret = getNextClaims(prevSensor, &claim);
                uassert(ret == 0);

                Reply(sensorCourierTid, &claim, sizeof(claim));
                sensorCourierTid = 0;
            }

            break;
        }

        // Command worker: I've just issued a set speed command!
        case commandWorkerSpeedSet:
        {
            Reply(tid, 0, 0);
            prevSpeed = speed;
            speed = (char)(message.data.setSpeed.speed);
            printf(COM2, "Speed set: %d, currentState: %d\n\r", speed, state);

            if (state == Ready || state == Reversing) break;
            else if (state == Stopped && speed > 0) state = Accelerating;
            else if (state == Running && speed == 0) state = Decelerating;
            else if (state == Decelerating && speed > 0) state = Accelerating;
            else if (state == Accelerating && speed == 0) state = Decelerating;
            break;
        }
        /**
        The go command tells the engineer to navigate to a certain
        location. To accomplish this, the engineer needs a path.
        Engineer asks for a path from where the engineer is currently
        at, to the go node.
        The engineer handles a sequence of instructions. An enstruction
        represents a single destination target, containing a list of
        track nodes that the engineer may need to reserve while enroute
        to the destination. And if the track node is a branch, there
        is information on which direction the branch should be switched
        to.
        Ebook contains a few enstructions.
        A sequence of short moves may consist of a series of
        enstructions, which at the end of each enstruction, two cases
        may exist: either (1) we reached the destination, or (2) if
        this isn't the case, then reverse command is required.
        */
        case go: {
            int node_number = message.data.go.index;
            track_node *dst = &g_track[node_number];
            // track_node *src = &g_track[37]; // 37=C6
            track_node *src = prevNode;
            printf(COM2, "Train %d goto %s from %s\n\r",
                trainNumber, dst->name, src->name);
            Reply(tid, 0, 0);

            /**
            Engineer should convert the destination to a Ebook.
            */
            PathBuffer pb;
            int ret = planRoute(src, dst, &pb);
            if (ret < 0) {
                printf(COM2, "GO: planRoute bad %d\n\r", ret);
                initEbook(&ebook);
                break;
            }
            printPath(&pb);
            /**
            expandPath is designed to be useful when used to do reservations
            with reverse moves (short moves)
            */
            // expandPath(&pb);
            // printPath(&pb);
            makeEbook(&pb, &ebook);
            printEbook(&ebook);
            /**
            When the ebook's length > 0, it is considered valid.
            Engineer always keeps his finger on which landmark he travel past.

            He leverages the existing X-marks-the-spot command and uses that
            to stop on his landmark.
            His only new responsibility is to correctly switch any turnouts
            that is on his path.

            -- Reservations --
            Before reservation is added, he presumes all the track belongs
            to him. Later, when reservation is added, he is a bit more reserved.

            -- Reverse pathing --
            For now he assumes there is no reverse pathing. Consequently,
            it is sufficient to only look at the first enstruction.
            */


            /**
            Assume he owns the path to the destination, and no reverseing.
            Correctly switch any turnouts to the desired curvature.
            */
            Enstruction *first = &(ebook.enstructs[0]);
            for (int i = 0; i < first->length; i++) {
                printf(COM2, "%s %d\n\r",
                    first->tracknodes[i]->name, first->turnops[i]);
                if (first->turnops[i]) {
                    int branch_idx = turnopGetTracknodeIndex(first->turnops[i]);
                    int turnout_number = g_track[branch_idx].num;
                    bool curve = turnopGetCurve(first->turnops[i]);
                    printf(COM2, "Set %d to %c\r\n",
                        turnout_number, curve ? 'c' : 's');
                    setTurnout(turnout_number, curve ? 'c' : 's');
                }
            }

            /**
            Leverage the use of X-marks-the-spot.
            */
            track_node *destination = first->togo.node;
            targetNode = destination;
            printf(COM2, "X-marks-the-spot %s\r\n", destination->name);
            targetOffset = 0;


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

