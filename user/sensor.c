#include <user/sensor.h>
#include <utils.h>
#include <priority.h>
#include <user/vt100.h>
#include <user/syscall.h>
#include <debug.h> // assert
#include <user/train.h> // halting
#include <user/trackserver.h>
#include <user/nameserver.h>
#include <user/track_data.h>
#include <user/pathfinding.h>

#define NUM_SENSORS         5
#define SENSOR_RESET        192
#define SENSOR_QUERY        (128 + NUM_SENSORS)
#define NUM_RECENT_SENSORS  7
#define SENSOR_BUF_SIZE     128
#define MAX_NUM_ENGINEER    2

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
        assert(0 <= i && i < NUM_RECENT_SENSORS);
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

void pushSensorData(SensorMessage *message, IBuffer *sensorBuf, IBuffer *timeBuf)
{
    char data = message->data;
    char offset = ((message->seq % 2 == 0) ? 0 : 8);
    int time = message->time;

    // loop over the byte
    char i, index;
    for (i = 0, index = 8; i < 8; i++, index--)
    {
        if ((1 << i) & data)
        {
            // calculate group and number
            int group = message->seq / 2;
            int number = index + offset;


            // encode sensor and push into buffers
            int sensor = (group << 8) | number;
            IBufferPush(sensorBuf, sensor);
            IBufferPush(timeBuf, time);
        }
    }
}

void sensorHalt(int train_number, char sensor_group, int sensor_number) {
    // gets called by the parser
    assert(0 < train_number && train_number < 80);
    assert('a' <= sensor_group && sensor_group <= 'e');
    assert(1 <= sensor_number && sensor_number <= 16);

    int group = sensor_group - 'a';
    halt_train_number = train_number;
    setSensorData(&halt_reading, group, sensor_number);
}

void sensorTime(struct SensorData *sensor1, struct SensorData *sensor2) {
    time_sensor_pair.sensor1_group = sensor1->group;
    time_sensor_pair.sensor1_offset = sensor1->offset;
    time_sensor_pair.sensor2_group = sensor2->group;
    time_sensor_pair.sensor2_offset = sensor2->offset;
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
    req.type = MESSAGE_SENSOR_WORKER;

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
                req.data.sm.data = c;
                req.data.sm.seq = i;
                req.data.sm.time = timestamp;
                Send(pid, &req, sizeof(req), 0, 0);
            }
            else
            {
                sensorStates[i] = 0;
            }
        }
    }
}

// the engineer "claims" upcoming sensors via the sensor courier. When the sensor actually hits and
// matches a claim, the sensor is "attributed" to that train, and set to the prevSensor
typedef struct {
    // tells if the sensor server could reply directly to tid
    char isBlocked;

    // the tid of the sensor courier to reply to
    int tid;

    // the time when prevSensor is hit
    int prevTime;

    // the prevSensor that is hit by the particular engineer
    int prevSensor;

    // the next sensor that the engineer expects to hit
    int primaryClaim;

    // the sensor that the engineer expects to hit if the primary sensor fails / turnout failure
    int secondaryClaim;

    // when each of the claimed sensors is expected to be triggered
    // int primaryTime;
    // int secondaryTime;
} Attribution;

static inline
void initAttribution(Attribution attrs[])
{
    for (int i = 0; i < MAX_NUM_ENGINEER; i++)
    {
        attrs[i].isBlocked = 0;
        attrs[i].tid = 0;
        attrs[i].prevTime = 0;
        attrs[i].prevSensor = 0;
        attrs[i].primaryClaim = 0;
        attrs[i].secondaryClaim = 0;
        // attrs[i].primaryTime = 0;
        // attrs[i].secondaryTime = 0;
    }
}

Attribution* getAttribution(int tid, int *numEngineer, Attribution attrs[])
{
    assert(*numEngineer <= MAX_NUM_ENGINEER);
    for (int i = 0; i < *numEngineer; i++)
    {
        if (attrs[i].tid == tid)
        {
            // found one; returning it
            return &attrs[i];
        }
    }

    if (*numEngineer == MAX_NUM_ENGINEER)
    {
        // attribution buffer is full and did not find the tid; returning NULL
        uassert(0);
        return 0;
    }

    // adding the tid into the list
    Attribution *attribution = &attrs[(*numEngineer)++];
    attribution->tid = tid;
    printf(COM2, "attribution created for new engineer courier tid: %d, numEngineer: %d\n\r", tid, *numEngineer);
    return attribution;
}

void sensorServer()
{
    int tid = 0;
    SensorRequest req;
    int sb[SENSOR_BUF_SIZE];
    int tb[SENSOR_BUF_SIZE];
    IBuffer sensorBuf;
    IBuffer timeBuf;
    IBufferInit(&sensorBuf, sb, SENSOR_BUF_SIZE);
    IBufferInit(&timeBuf, tb, SENSOR_BUF_SIZE);

    int numEngineer = 0;
    Attribution attrs[MAX_NUM_ENGINEER];
    initAttribution(attrs);

    RegisterAs("sensorServer");

    // create the sensor courier
    Create(PRIORITY_SENSOR_COURIER, sensorWorker);

    for (;;)
    {
        Receive(&tid, &req, sizeof(req));

        switch (req.type)
        {
            case MESSAGE_SENSOR_WORKER:
            {
                SensorMessage *message = &(req.data.sm);

                // get the byte and the timestamp
                char data = message->data;
                char offset = ((message->seq % 2 == 0) ? 0 : 8);
                int timestamp = message->time;

                // loop over the byte
                char i, index;
                for (i = 0, index = 8; i < 8; i++, index--)
                {
                    if ((1 << i) & data)
                    {
                        // calculate the nodeIndex from group & number
                        int sensorIndex = getSensorIndex(message->seq / 2,
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
                            if (attr->primaryClaim == sensorIndex)
                            {
                                if (minPrimaryTime == -1 ||
                                    attr->prevTime < minPrimaryTime)
                                {
                                    minPrimaryIndex = i;
                                    minPrimaryTime = attr->prevTime;
                                }
                            }
                            else if (attr->secondaryClaim == sensorIndex)
                            {
                                if (minSecondaryTime == -1 ||
                                    attr->prevTime < minSecondaryTime)
                                {
                                    minSecondaryIndex = i;
                                    minSecondaryTime = attr->prevTime;
                                }
                            }
                        }

                        int attrIndex = (minPrimaryIndex > 0) ? minPrimaryIndex : minSecondaryIndex;

                        if (attrIndex > 0)
                        {
                            Attribution *attribution = &attrs[attrIndex];

                            while (attribution->isBlocked == 0)
                            {
                                // MAYBE TODO: Implement a reply queue; if the courier
                                // is not blocked, add it into the reply queue
                                printf(COM2, "Warning: sensor server calling Pass() - %d not send blocked\n\r", attribution->tid);
                                Pass();
                            }

                            // preparing reply to the sensor courier
                            SensorUpdate su;
                            su.sensor = sensorIndex;
                            su.time   = timestamp;

                            // reply to the courier
                            Reply(attribution->tid, &su, sizeof(su));
                            attribution->isBlocked = 0;

                            // set prevTime & prevSensor, clear the claims
                            attribution->prevTime = timestamp;
                            attribution->prevSensor = sensorIndex;
                            attribution->primaryClaim = 0;
                            attribution->secondaryClaim = 0;
                        }
                        else
                        {
                            // spurious sensor hit: ignore
                            printf(COM2, "spurious sensor hit\n\r");
                        }
                    }
                }

                // unblock the sensor worker
                Reply(tid, 0, 0);

                break;
            }
            case MESSAGE_SENSOR_COURIER:
            {
                // get the attribution pointer
                Attribution *attribution = getAttribution(tid, &numEngineer, attrs);
                assert(attribution != 0);

                // set the primary and secondary claim
                attribution->isBlocked = 1;
                attribution->primaryClaim = req.data.sc.primaryClaim;
                attribution->secondaryClaim = req.data.sc.secondaryClaim;
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
