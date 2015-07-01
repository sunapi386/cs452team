#include <user/sensor.h>
#include <utils.h>
#include <priority.h>
#include <user/vt100.h>
#include <user/syscall.h>
#include <debug.h> // assert
#include <user/train.h> // halting

#define NUM_SENSORS     5
#define SENSOR_RESET    192
#define SENSOR_QUERY    (128 + NUM_SENSORS)
#define NUM_RECENT_SENSORS    7
#define POS                     "\033[4;24H"
#define POS1                    "\033[;24H"


static char *trackA =
"\033[4;24H"    "=========S===S===================================\\"
"\033[5;24H"    "=======S/   /  ==========S========S============\\  \\"
"\033[6;24H"    "======/    S==/           \\   |  /              \\  \\"
"\033[7;24H"    "          /                \\  |S/                \\==S"
"\033[8;24H"    "         |                  \\S|                     |"
"\033[9;24H"    "         |                    |S\\                   |"
"\033[10;24H"   "          \\                 /S|  \\               /==S"
"\033[11;24H"   "           S==\\            /  |   \\             /  /"
"\033[12;24H"   "========\\   \\  ==========S=========S===========/  /"
"\033[13;24H"   "=========S\\  \\=========S============S============/"
"\033[14;24H"   "===========S\\           \\          /"
"\033[15;24H"   "=============S===========S========S============";

static char *trackB =
"\033[4;24H"    "=========S===S===================================\\"
"\033[5;24H"    "=======S/   /  ==========S========S============\\  \\"
"\033[6;24H"    "      /    S==/           \\   |  /              \\  \\"
"\033[7;24H"    "   /=/    /                \\  |S/                \\==S"
"\033[8;24H"    "   |     |                  \\S|                     |"
"\033[9;24H"    "   |     |                    |S\\                   |"
"\033[10;24H"   "   \\=     \\                 /S|  \\               /==S"
"\033[11;24H"   "     \\=\\   S==\\            /  |   \\             /  /"
"\033[12;24H"   "        \\   \\  ==========S=========S===========/  /"
"\033[13;24H"   "=========S\\  \\=========S============S============/"
"\033[14;24H"   "===========S\\           \\          /"
"\033[15;24H"   "=============S===========S========S============";


typedef struct SensorReading {
    char group; // 0 to 4
    char offset; // 1 to 16
} SensorReading;

static void setSensorReading(SensorReading *s, char group, char offset) {
    // compiler hax to silence comparison of char is always true in limited range
    int newgroup = group * 1000;
    assert(0 <= newgroup && newgroup <= 4 * 1000);
    assert(1 <= (unsigned) offset && (unsigned) offset <= 16);
    s->group = group;
    s->offset = offset;
}

static SensorReading recent_sensors[NUM_RECENT_SENSORS];

static char sensor_states[2 * NUM_SENSORS];
static int last_byte = 0;
static int recently_read = 0;

static int halt_train_number;
static SensorReading halt_reading;

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

static void _sensorFormat(String *s, const SensorReading *sensorReading) {
    if(sensorReading->offset == 0) {
        return;
    }
    // e.g. formats the SensorReading to "A10"
    char alpha = sensorReading->group;
    char bit = sensorReading->offset;
    // Mmm alphabit soup
    sputc(s, 'A' + alpha);
    sputc(s, '.');
    sputuint(s, bit, 10);
    sputstr(s, "         \r\n");
}

static void _updateSensoryDisplay() {
    String s;
    sinit(&s);
    sputstr(&s, VT_CURSOR_SAVE);
    vt_pos(&s, VT_SENSOR_ROW + 1, VT_SENSOR_COL);

    for(int i = recently_read ; ; ) {
        _sensorFormat(&s, &recent_sensors[i]);
        i = (i + 1) % NUM_RECENT_SENSORS;
        if(i == recently_read) {
            break;
        }
    }

    sputstr(&s, VT_RESET);
    sputstr(&s, VT_CURSOR_RESTORE);
    PutString(COM2, &s);
}


static inline void _handleChar(char c, int reply_index) {
    sensor_states[last_byte] = c;
    //char diff = c;

    char offset = ((reply_index % 2 == 0) ? 0 : 8);
    char i, index;
    for (i = 0, index = 8; i < 8; i++, index--) {
        if ((1 << i) & c) {
            int group_number = last_byte / 2;
            int group_offset = index + offset;
            setSensorReading(&recent_sensors[recently_read], group_number, group_offset);
            _updateSensoryDisplay();
            recently_read = (recently_read + 1) % NUM_RECENT_SENSORS;
            // check if reading was what we should halt on
            if(halt_reading.group == group_number && halt_reading.offset == group_offset) {
                trainSetSpeed(halt_train_number, 0);
                printf(COM2, "HALTING TRAIN %d BECAUSE SENSOR TRIGGERED", halt_train_number);
            }
        }
    }

    last_byte = (last_byte + 1) % (2 * NUM_SENSORS);
}

static void sensorTask() {
    for(int i = 0; i < NUM_RECENT_SENSORS; i++) {
        recent_sensors[i].group = recent_sensors[i].offset = 0;
    }
    for(int i = 0; i < 2 * NUM_SENSORS; i++) {
        sensor_states[i] = 0;
    }
    last_byte = recently_read = 0;
    initDrawSensorArea();
    drawTrackLayoutGraph(A);
    Putc(COM1, SENSOR_RESET);

    while(1) {
        Putc(COM1, SENSOR_QUERY);
        for(int i = 0; i < (2 * NUM_SENSORS); i++) {
            char c = Getc(COM1);
            if(c != 0) {
                last_byte = i;
                _handleChar(c, i);
            }
        }
    }

}

void initSensor() {
    debug("initSensor");
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
    int bit = sensor_number % 8;

    printf(COM2, "train #%d group %d offset bit %d\r\n", train_number, group, bit);

    halt_train_number = train_number;
    setSensorReading(&halt_reading, group, bit);
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
