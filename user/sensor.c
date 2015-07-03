#include <user/sensor.h>
#include <utils.h>
#include <priority.h>
#include <user/vt100.h>
#include <user/syscall.h>
#include <debug.h> // assert
#include <user/train.h> // halting
#include <user/trackserver.h>
#include <user/nameserver.h>

#define NUM_SENSORS     5
#define SENSOR_RESET    192
#define SENSOR_QUERY    (128 + NUM_SENSORS)
#define NUM_RECENT_SENSORS    7

static char *trackA =
"\033[2;44H"    "-- Track A --"
"\033[4;24H"    "---------12--11----------------------------------\\"
"\033[5;24H"    "-------4/   /  ---------13--------10-----------\\  \\"
"\033[6;24H"    "          14--/           \\   |  /              \\  \\"
"\033[7;24H"    "          /                \\  |/155              \\--9"
"\033[8;24H"    "         |                156\\|                    |"
"\033[9;24H"    "         |                    |\\154                |"
"\033[10;24H"   "          \\               153/|  \\               /--8"
"\033[11;24H"   "          15--\\            /  |   \\             /  /"
"\033[12;24H"   "--------\\   \\  ----------S---------S-----------/  /"
"\033[13;24H"   "---------1\\  \\---------6------------7------------/"
"\033[14;24H"   "-----------2\\           \\          /"
"\033[15;24H"   "-------------3-----------18--------5------------";

static char *trackB =
"\033[2;44H"    "-- Track B --"
"\033[4;24H"    "---------12--11----------------------------------\\"
"\033[5;24H"    "-------4/   /  ---------13--------10-----------\\  \\"
"\033[6;24H"    "      /   14--/           \\   |  /              \\  \\"
"\033[7;24H"    "   /-/    /                \\  |/155               \\--9"
"\033[8;24H"    "   |     |                156\\|                     |"
"\033[9;24H"    "   |     |                    |\\154                 |"
"\033[10;24H"   "   \\-     \\               153/| \\                /--8"
"\033[11;24H"   "     \\-\\  15--\\            /  |  \\              /  /"
"\033[12;24H"   "        \\   \\  ---------16--------17-----------/  /"
"\033[13;24H"   "---------1\\  \\---------6------------7------------/"
"\033[14;24H"   "-----------2\\           \\          /"
"\033[15;24H"   "-------------3-----------18--------5------------";

static SensorData recent_sensors[NUM_RECENT_SENSORS];

static char sensor_states[2 * NUM_SENSORS];
static int last_byte = 0;
static int recently_read = 0;

static struct {
    char sensor1_group;
    char sensor1_offset;
    char sensor2_group;
    char sensor2_offset;
    int start_time;
} time_sensor_pair;

static int halt_train_number;
static SensorData halt_reading;

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
        sensorFormat(&s, &recent_sensors[i]);
        i = (i + 1) % NUM_RECENT_SENSORS;
        if(i == recently_read) {
            break;
        }
    }

    sputstr(&s, VT_RESET);
    sputstr(&s, VT_CURSOR_RESTORE);
    PutString(COM2, &s);
}

static void sensorCourierTask() {
    int parent = MyParentTid();
    int engineerTask = WhoIs("engineer");
    assert(engineerTask >= 0);
    struct sensorAndTimestamp;
    int sensor;
    for(;;) {
        Send(parent, 0, 0, &sensor, sizeof(int));
        Send(engineerTask, &sensor, sizeof(int), 0, 0);
    }
}

typedef struct SensorAndTimestamp {
    int index_to_train_data;
    int time;
} SensorAndTimestamp;

static void notifyEngineers(const char group, const char offset, const int time) {
    int index_to_train_data = 16 * group + offset - 1;
    SensorAndTimestamp st = {index_to_train_data, time};
    (void)st;
}


static inline void handleChar(char c, int reply_index) {
    sensor_states[last_byte] = c;

    char offset = ((reply_index % 2 == 0) ? 0 : 8);
    char i, index;
    for (i = 0, index = 8; i < 8; i++, index--) {
        if ((1 << i) & c) {
            int group_number = last_byte / 2;
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
                debug("sensor1 triggered on %c%d", group_number, group_offset);
                time_sensor_pair.start_time = Time();
            }
            // if sensor2 was hit, calculate difference from start_time
            if(time_sensor_pair.sensor2_group == group_number &&
               time_sensor_pair.sensor2_offset == group_offset) {
                int time_diff = Time() - time_sensor_pair.start_time;
                printf(COM2, "sensor2 %c%d to sensor %c%d took %d ticks\r\n",
                    time_sensor_pair.sensor1_group + 'A',
                    time_sensor_pair.sensor1_offset,
                    time_sensor_pair.sensor2_group + 'A',
                    time_sensor_pair.sensor2_offset,
                    time_diff);
            }
        }
    }

    last_byte = (last_byte + 1) % (2 * NUM_SENSORS);
}

static void sensorCourier()
{

}

static void sensorTask() {
    initDrawSensorArea();
    drawTrackLayoutGraph(A);
    Putc(COM1, SENSOR_RESET);

    while(1) {
        Putc(COM1, SENSOR_QUERY);
        for(int i = 0; i < (2 * NUM_SENSORS); i++) {
            char c = Getc(COM1);
            if(c != 0) {
                last_byte = i;
                handleChar(c, i);
            }
        }
    }

}

void initSensor() {
    for(int i = 0; i < NUM_RECENT_SENSORS; i++) {
        recent_sensors[i].group = recent_sensors[i].offset = 0;
    }
    for(int i = 0; i < 2 * NUM_SENSORS; i++) {
        sensor_states[i] = 0;
    }
    last_byte = 0;
    recently_read = 0;
    halt_train_number = 0;
    halt_reading.group = halt_reading.offset = 0;
    time_sensor_pair.sensor1_group = 0;
    time_sensor_pair.sensor1_offset = 0;
    time_sensor_pair.sensor2_group = 0;
    time_sensor_pair.sensor2_offset = 0;
    time_sensor_pair.start_time = 0;

    assert(STR_MAX_LEN > strlen(trackA));
    assert(STR_MAX_LEN > strlen(trackB));
    Create(PRIORITY_SENSOR_TASK, sensorTask);
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

void drawTrackLayoutGraph(Track which_track) {
    String s;
    sinit(&s);
    sputstr(&s, VT_CURSOR_SAVE);
    vt_pos(&s, VT_TRACK_GRAPH_ROW, VT_TRACK_GRAPH_COL);
    sputstr(&s, VT_RESET);
    sputstr(&s, which_track == A ? trackA : trackB);
    sputstr(&s, VT_RESET);
    sputstr(&s, VT_CURSOR_RESTORE);
    PutString(COM2, &s);
}
