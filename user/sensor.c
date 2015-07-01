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
#define NUM_RECENT_SENSORS    8
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
    char group;
    char offset;
} SensorReading;

static SensorReading recent_sensors[NUM_RECENT_SENSORS];

static char sensor_states[2 * NUM_SENSORS];
static int last_byte = 0;
static int recently_read = 0;

static int halt_train_number;
static SensorReading halt_reading;

static void _sensorFormat(String *s, SensorReading *sensorReading) {
    // e.g. formats the SensorReading to "A10"
    char alpha = sensorReading->group / 2;
    char bit = sensorReading->offset;
    // Mmm alphabit soup
    sprintfstr(s, "%c%d\r\n", 'A' + alpha, bit);
}

static void drawSensorArea() {
    String s;
    sinit(&s);
    sprintf(&s, VT_CURSOR_SAVE);
    vt_pos(&s, VT_SENSOR_ROW, VT_SENSOR_COL);
    sputstr(&s, VT_RESET);
    sprintfstr(&s, "-- RECENT SENSORS --\r\n");
    for(int i = 1; i <= NUM_RECENT_SENSORS; i++) {
        sprintfstr(&s, "%d. A10\r\n", i);
    }
    sprintfstr(&s, "%s%s", VT_RESET, VT_CURSOR_RESTORE);
    PutString(COM2, &s);
}

static void _updateSensoryDisplay() {
    String s;
    sinit(&s);
    sputstr(&s, VT_CURSOR_SAVE);
    // TODO: calc and determine where the actual row and col is, on gui
    vt_pos(&s, VT_SENSOR_ROW + 1, VT_SENSOR_COL + sizeof("0. "));
    sputstr(&s, VT_RESET);

    for(int i = (recently_read + 1) % NUM_SENSORS;
        recent_sensors[recently_read].offset != 0;
        i = (i + 1) % NUM_RECENT_SENSORS) {
        _sensorFormat(&s, &recent_sensors[i]);
    }

    sprintfstr(&s, "%s%s", VT_RESET, VT_CURSOR_RESTORE);
    PutString(COM2, &s);
}


static inline void _handleChar(char c) {
    char old = sensor_states[last_byte];
    sensor_states[last_byte] = c;
    char diff = ~old & c;
    int group = 0;
    // make sensor reading for each different (changed) bit
    for(int bit = 1; diff; diff >>= 1, bit++) {
        if(diff & 1) {
            recent_sensors[group].group = last_byte;
            recent_sensors[group].offset = bit;

            group = (group - 1 + NUM_RECENT_SENSORS) % NUM_RECENT_SENSORS;

            // check if reading was what we should halt on
            if(halt_reading.group == last_byte &&
                halt_reading.offset == bit) {
                debug("HALTING TRAIN BECAUSE SENSOR TRIGGERED");
                trainSetSpeed(halt_train_number, 0);
            }

            recent_sensors[group].group = 0;
            recent_sensors[group].offset = 0;

            _updateSensoryDisplay();

            // TODO: later, notify the train controller of sensor trigger

        }
    }

    last_byte = (last_byte + 1) % (2 * NUM_SENSORS);
}

static void _sensorTask() {
    for(int i = 0; i < NUM_RECENT_SENSORS; i++) {
        recent_sensors[i].group = recent_sensors[i].offset = 0;
    }
    for(int i = 0; i < 2 * NUM_SENSORS; i++) {
        sensor_states[i] = 0;
    }
    last_byte = recently_read = 0;
    drawSensorArea();
    drawTrackLayoutGraph(A);
    Putc(COM1, SENSOR_RESET);

    while(1) {
        Putc(COM1, SENSOR_QUERY);
        for(int i = 0; i < (2 * NUM_SENSORS); i++) {
            char c = Getc(COM1);
            if(c != 0) {
                _handleChar(c);
            }
        }
    }

}

void initSensor() {
    assert(STR_MAX_LEN > strlen(trackA));
    assert(STR_MAX_LEN > strlen(trackB));
    Create(PRIORITY_SENSOR_TASK, _sensorTask);
}

void sensorHalt(int train_number, char sensor_group, int sensor_number) {
    // gets called by the parser
    assert(0 < train_number && train_number < 80);
    assert('a' <= sensor_group && sensor_group <= 'e');
    assert(1 <= sensor_number && sensor_number <= 16);

    int group = (sensor_group - 'a') * 2;
    if(sensor_number > 8 ) {
        sensor_number -= 8;
        group++;
    }
    int bit = 9 - sensor_number;

    debug("train #%d group %d offset %d\r\n", train_number, group, bit);

    halt_train_number = train_number;
    halt_reading.group = group;
    halt_reading.offset = bit;
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
