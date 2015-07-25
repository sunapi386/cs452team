#include <user/sensor.h>
#include <utils.h>
#include <priority.h>
#include <user/vt100.h>
#include <user/syscall.h>
#include <debug.h>
#include <user/train.h>
#include <user/engineer.h>
#include <user/trackserver.h>
#include <user/nameserver.h>
#include <user/track_data.h>
#include <user/pathfinding.h>

#define NUM_SENSORS         5
#define SENSOR_RESET        192
#define SENSOR_QUERY        (128 + NUM_SENSORS)
#define NUM_RECENT_SENSORS  7
#define SENSOR_BUF_SIZE     128

// the engineer "claims" upcoming sensors via the sensor courier. When the sensor actually hits and
// matches a claim, the sensor is "attributed" to that train, and set to the prevSensor
typedef struct {
    // the tid of the sensor courier to reply to
    int courierTid;

    // the tid of the engineer that is supposed to be used by the engineer courier
    int engineerTid;

    // the time when prevSensor is hit
    int prevTime;

    // the prevSensor that is hit by the particular engineer
    int prevSensor;

    // the next sensor that the engineer expects to hit
    int primary;

    // the sensor that the engineer expects to hit if the primary sensor fails / turnout failure
    int secondary;

    // when each of the claimed sensors is expected to be triggered
    // int primaryTime;
    // int secondaryTime;
} Attribution;

static struct {
    char sensor1_group;
    char sensor1_offset;
    char sensor2_group;
    char sensor2_offset;
    int start_time;
} time_sensor_pair;

static int recently_read = 0;
static SensorData recent_sensors[NUM_RECENT_SENSORS];

static int halt_train_number;
static SensorData halt_reading;

static inline int getSensorIndex(int group, int offset) {
    return 16 * group + offset - 1;
}

static void initDrawSensorArea() {
    String s;
    sinit(&s);
    sprintf(&s, VT_CURSOR_SAVE);
    vt_pos(&s, VT_SENSOR_ROW, VT_SENSOR_COL);
    sputstr(&s, VT_RESET);
    sputstr(&s, "-- RECENT SENSORS --\r\n");
    sputstr(&s, VT_RESET);
    sputstr(&s, VT_CURSOR_RESTORE);
    PutString(COM2, &s);
}

static void setSensorData(SensorData *s, char group, char offset) {
    // compiler hax to silence comparison of char is always true in limited range
    int newgroup = group * 1000;
    assert(0 <= newgroup && newgroup <= 4 * 1000);
    assert(1 <= (unsigned) offset && (unsigned) offset <= 16);
    s->group = group;
    s->offset = offset;
}

static void sensorFormat(String *s, const SensorData *sensorReading) {
    if(sensorReading->offset == 0) {
        return;
    }
    // e.g. formats the SensorData to "A10"
    char alpha = sensorReading->group;
    char bit = sensorReading->offset;
    // Mmm alphabit soup
    sputstr(s, "      ");
    sputc(s, 'A' + alpha);
    sputc(s, '.');
    sputuint(s, bit, 10);
    sputstr(s, "      \r\n");
}

static void updateSensoryDisplay() {
    String s;
    sinit(&s);
    sputstr(&s, VT_CURSOR_SAVE);
    vt_pos(&s, VT_SENSOR_ROW + 1, VT_SENSOR_COL);

    for(int i = recently_read ; ; ) {
        uassert(0 <= i && i < NUM_RECENT_SENSORS);
        sensorFormat(&s, &recent_sensors[i]);
        i = (i == 0) ? (NUM_RECENT_SENSORS - 1) : (i - 1);
        if(i == recently_read) {
            break;
        }
    }

    sputstr(&s, VT_RESET);
    sputstr(&s, VT_CURSOR_RESTORE);
    PutString(COM2, &s);
}

static inline void handleChar(char c, int reply_index) {
    char offset = ((reply_index % 2 == 0) ? 0 : 8);
    char i, index;
    for (i = 0, index = 8; i < 8; i++, index--) {
        if ((1 << i) & c) {
            int group_number = reply_index / 2;
            int group_offset = index + offset;
            setSensorData(&recent_sensors[recently_read], group_number, group_offset);
            updateSensoryDisplay();
            recently_read = (recently_read + 1) % NUM_RECENT_SENSORS;

            // check if reading was what we should halt on
            if(halt_reading.group == group_number && halt_reading.offset == group_offset) {
                trainSetSpeed(halt_train_number, 0);
            }

            // if sensor1 was hit, mark the start_time
            if(time_sensor_pair.sensor1_group == group_number &&
               time_sensor_pair.sensor1_offset == group_offset) {
                time_sensor_pair.start_time = Time();
            }
            // if sensor2 was hit, calculate difference from start_time
            if(time_sensor_pair.sensor2_group == group_number &&
               time_sensor_pair.sensor2_offset == group_offset) {
                int time_diff = Time() - time_sensor_pair.start_time;
                printf(COM2, "time between %c%d and %c%d is %d",
                    time_sensor_pair.sensor1_group + 'A',
                    time_sensor_pair.sensor1_offset,
                    time_sensor_pair.sensor2_group + 'A',
                    time_sensor_pair.sensor2_offset,
                    time_diff);
            }
        }
    }
}

void sensorHalt(int train_number, char sensor_group, int sensor_number) {
    // gets called by the parser
    uassert(0 < train_number && train_number < 80);
    uassert('a' <= sensor_group && sensor_group <= 'e');
    uassert(1 <= sensor_number && sensor_number <= 16);

    int group = sensor_group - 'a';
    halt_train_number = train_number;
    setSensorData(&halt_reading, group, sensor_number);
}

static void clearScreen()
{
    String s;
    sinit(&s);
    sputstr(&s, VT_CLEAR_SCREEN);
    PutString(COM2, &s);
}

void sensorWorker()
{
    Putc(COM1, SENSOR_RESET);

    int pid = MyParentTid();
    SensorRequest req;
    char sensorStates[2 * NUM_SENSORS];

    clearScreen();
    initDrawSensorArea();

    // initialize sensor states
    for (int i = 0; i < 2 * NUM_SENSORS; i++)
    {
        sensorStates[i] = 0;
    }

    // start courier main loop
    req.type = newSensor;

    for (;;)
    {
        Putc(COM1, SENSOR_QUERY);
        int timestamp = 0;
        for (int i = 0; i < 2 * NUM_SENSORS; i++)
        {
            char c = Getc(COM1);
            if (c != 0)
            {
                // ignore sensors that are previously triggered
                char old = sensorStates[i];
                sensorStates[i] = c;
                c = ~old & c;
                if (c == 0) continue;

                // grab timestamp
                if (timestamp == 0)
                {
                    timestamp = Time();
                }

                // update UI
                handleChar(c, i);

                // send request to sensor server
                req.data.newSensor.data = c;
                req.data.newSensor.seq = i;
                req.data.newSensor.time = timestamp;
                Send(pid, &req, sizeof(req), 0, 0);
            }
            else
            {
                sensorStates[i] = 0;
            }
        }
    }
}

// the task that communicates from sensor server to the engineer to deliver sensor updates
// or sensor timeout notifications
void engineerCourier()
{
    // get the tid of the sensor server
    int sensorServerTid = MyParentTid();

    SensorRequest sensorRequest;     // courier -> sensor server
    SensorDelivery delivery;         // sensor server -> courier
    EngineerMessage engineerMessage; // courier -> engineer

    // setup messages
    sensorRequest.type = requestSensor;
    engineerMessage.type = updateSensor;

    // main loop
    for (;;)
    {
        // send to and block on sensor server
        // when the sensor server wants to send an update, it replies with either:
        Send(sensorServerTid, &sensorRequest, sizeof(sensorRequest), &delivery, sizeof(delivery));

        // populate engineer message
        // TODO: support for timeout message type
        int engineerTid = delivery.tid;
        uassert(engineerTid > 0 && delivery.type == SENSOR_TRIGGER);
        engineerMessage.data.updateSensor.sensor = delivery.nodeIndex;
        engineerMessage.data.updateSensor.time = delivery.timestamp;

        // send to the engineer (and immedietly gets unblocked)
        Send(engineerTid, &engineerMessage, sizeof(engineerMessage), 0, 0);
    }
}

static inline
void setSensorDelivery(SensorDelivery *delivery, int tid, int type, int nodeIndex, int timestamp)
{
    delivery->tid = tid;
    delivery->type = type;
    delivery->nodeIndex = nodeIndex;
    delivery->timestamp = timestamp;
}

static inline
void initAttribution(Attribution attrs[])
{
    for (int i = 0; i < MAX_NUM_ENGINEER; i++)
    {
        attrs[i].courierTid = -1;
        attrs[i].engineerTid = -1;
        attrs[i].prevTime = -1;
        attrs[i].prevSensor = -1;
        attrs[i].primary = -1;
        attrs[i].secondary = -1;
        // attrs[i].primaryTime = 0;
        // attrs[i].secondaryTime = 0;
    }
}

static inline
void initialAttribution(int courierTid,
                        SensorRequest *sensorRequest,
                        int *numEngineer,
                        Attribution attrs[])
{
    uassert(*numEngineer < MAX_NUM_ENGINEER);

    // get pointer to attribution
    Attribution *attr = &attrs[(*numEngineer)++];

    uassert(attr->primary == -1 && attr->prevSensor == -1);

    // set new values
    attr->courierTid = courierTid;
    attr->engineerTid = sensorRequest->data.initialClaim.engineerTid;
    attr->primary = sensorRequest->data.initialClaim.index;

    printf(COM2, "initial sensor for courierTid %d, engineerTid %d, attr->primary %d, numEngineer %d\n\r", courierTid, attr->engineerTid, attr->primary, *numEngineer);
}

Attribution *getAttribution(int courierTid, int numEngineer, Attribution attrs[])
{
    uassert(numEngineer <= MAX_NUM_ENGINEER);

    for (int i = 0; i < numEngineer; i++)
    {
        if (attrs[i].courierTid == courierTid)
        {
            return &attrs[i];
        }
    }
    return 0;
}

extern track_node g_track[TRACK_MAX];

int setAttribution(int courierTid, SensorClaim *claim, int numEngineer, Attribution attrs[])
{
    // get the attribution of the specific courier
    Attribution *attr = getAttribution(courierTid, numEngineer, attrs);
    if (attr == 0)
    {
        uassert(0);
        return -1;
    }

    // set the desired values from sensor request
    attr->primary = claim->primary;
    attr->secondary = claim->secondary;

    const char *primaryName = "";
    const char *secondaryName = "";

    // FIXME: Remove
    if (attr->primary >= 0)
    {
        primaryName = g_track[attr->primary].name;
    }
    if (attr->secondary >= 0)
    {
        secondaryName = g_track[attr->secondary].name;
    }


    printf(COM2, "[setAttribution] indices: (%d, %d) primary: %s, secondary: %s\n\r", attr->primary, attr->secondary, g_track[attr->primary].name, g_track[attr->secondary].name);

    return 0;
}

void sensorServer()
{
    int tid = 0;
    int engineerCourierTid = 0;
    int numEngineer = 0;
    SensorRequest req;

    // Sensor queue is needed to store outgoing sensor messages
    // to engineer couriers
    SensorDelivery sensorBuf[64];
    SensorQueue sensorQueue;
    initSensorQueue(&sensorQueue, 64, &(sensorBuf[0]));

    Attribution attrs[MAX_NUM_ENGINEER];
    initAttribution(attrs);

    RegisterAs("sensorServer");

    // create the sensor worker & engineer courier
    Create(PRIORITY_SENSOR_COURIER, sensorWorker);
    Create(PRIORITY_ENGINEER_COURIER, engineerCourier);

    for (;;)
    {
        int len = Receive(&tid, &req, sizeof(req));
        uassert(len == sizeof(SensorRequest));

        switch (req.type)
        {
            // engineer courier requesting for sensor message to
            case requestSensor:
            {
                if (isSensorQueueEmpty(&sensorQueue))
                {
                    // case 1: no outgoing sensor; block it
                    engineerCourierTid = tid;
                }
                else
                {
                    // case 2: dequeue a sensor trigger and reply the engineer courier
                    uassert(engineerCourierTid == 0);
                    SensorDelivery sd;
                    int ret = dequeueSensor(&sensorQueue, &sd);
                    uassert(ret == 0);
                    Reply(tid, &sd, sizeof(sd));
                }
                break;
            }
            case newSensor:
            {
                // get the byte and the timestamp
                char data = req.data.newSensor.data;
                char offset = ((req.data.newSensor.seq % 2 == 0) ? 0 : 8);
                int timestamp = req.data.newSensor.time;

                // loop over the byte
                char i, index;
                for (i = 0, index = 8; i < 8; i++, index--)
                {
                    if ((1 << i) & data)
                    {
                        // calculate the nodeIndex from group & number
                        int sensorIndex = getSensorIndex(req.data.newSensor.seq / 2,
                                                         index + offset);

                        // loop over the attributions [looking for primary with the minimal prevTime]
                        int minPrimaryTime = -1;
                        int minPrimaryIndex = -1;
                        int minSecondaryTime = -1;
                        int minSecondaryIndex = -1;
                        for (int i = 0; i < numEngineer; i++)
                        {
                            // get the attribution info
                            Attribution *attr = &attrs[i];

                            // TODO: check for timeout
                            // if (timestamp - delta blah blah)

                            // TODO: handle initial attribution

                            // compute effective primary & secondary claims
                            if (attr->primary == sensorIndex)
                            {
                                if (minPrimaryTime == -1 ||
                                    attr->prevTime < minPrimaryTime ||
                                    attr->prevTime == -1)
                                {
                                    minPrimaryIndex = i;
                                    minPrimaryTime = attr->prevTime;
                                }
                            }
                            else if (attr->secondary == sensorIndex)
                            {
                                if (minSecondaryTime == -1 ||
                                    attr->prevTime < minSecondaryTime)
                                {
                                    minSecondaryIndex = i;
                                    minSecondaryTime = attr->prevTime;
                                }
                            }
                        }

                        int attrIndex = (minPrimaryIndex >= 0) ? minPrimaryIndex : minSecondaryIndex;

                        if (attrIndex >= 0)
                        {
                            Attribution *attribution = &attrs[attrIndex];
                            attribution->prevTime = timestamp;

                            // reply via engineer courier
                            if (engineerCourierTid != 0)
                            {
                                uassert(engineerCourierTid > 0);

                                // engineer courier blocked here, reply it directly
                                SensorDelivery delivery;
                                setSensorDelivery(&delivery,
                                                  attribution->engineerTid,
                                                  SENSOR_TRIGGER,
                                                  sensorIndex,
                                                  timestamp);

                                Reply(engineerCourierTid, &delivery, sizeof(delivery));
                                engineerCourierTid = 0;
                            }
                            else
                            {
                                // engineer courier not blocked, add it to sensor queue
                                enqueueSensor(&sensorQueue,
                                              attribution->engineerTid,
                                              SENSOR_TRIGGER,
                                              sensorIndex,
                                              timestamp);
                            }
                        }
                        else
                        {
                            // spurious sensor hit: ignore
                            printf(COM2, "spurious sensor hit. attrIndex: %d\n\r", attrIndex);
                        }
                    }
                }

                // unblock the sensor worker
                Reply(tid, 0, 0);

                break;
            }
            // Engineer -> sensor courier -> sensor server
            // Message contains: primary and secondary
            case claimSensor:
            {
                // reply sensor courier
                Reply(tid, 0, 0);

                int ret = setAttribution(tid, &(req.data.claimSensor), numEngineer, attrs);
                assert(ret == 0);
                break;
            }

            // Engineer -> sensor courier -> sensor server
            // Message contains: tid of engineer, node index of first sensor
            case initialClaim:
            {
                // reply sensor courier
                Reply(tid, 0, 0);

                // add a new attribution entry
                initialAttribution(tid, &req, &numEngineer, attrs);
                break;
            }
            default:
                uassert(0);
                break;
        }
    }
}

void initSensor() {
    recently_read = 0;
    halt_train_number = 0;
    halt_reading.group = halt_reading.offset = 0;
    time_sensor_pair.sensor1_group = 0;
    time_sensor_pair.sensor1_offset = 0;
    time_sensor_pair.sensor2_group = 0;
    time_sensor_pair.sensor2_offset = 0;
    time_sensor_pair.start_time = 0;
    for(int i = 0; i < NUM_RECENT_SENSORS; i++) {
        recent_sensors[i].group = 0;
        recent_sensors[i].offset = 0;
    }

    Create(PRIORITY_SENSOR_SERVER, sensorServer);
}
