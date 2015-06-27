#include <user/sensor.h>
#include <utils.h>
#include <priority.h>
#include <user/vt100.h>
#include <user/syscall.h>
#include <debug.h> // assert

#define NUM_SENSORS     5
#define SENSOR_RESET    192
#define SENSOR_QUERY    (128 + NUM_SENSORS)

#define NUM_RECENT_SENSORS    8

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
    sprintf(s, "%c%d", 'A' + alpha, bit);
}

static void _updateSensoryDisplay() {
    String s;
    sinit(&s);
    sprintf(&s, "%s%s" VT_CURSOR_HIDE, VT_CURSOR_SAVE);
    // TOOD: calc and determine where the actual row and col is, on gui
    vt_pos(&s, VT_SENSOR_ROW, VT_SENSOR_COL);
    sputstr(&s, VT_RESET);

    for(int i = (recently_read + 1) % NUM_SENSORS;
        recent_sensors[recently_read].offset != 0;
        i = (i + 1) % NUM_RECENT_SENSORS) {
        _sensorFormat(&s, &recent_sensors[i]);
    }

    sprintf(&s, "%s%s%s", VT_RESET, VT_CURSOR_RESTORE, VT_CURSOR_SHOW);
    PutString(COM2, &s);
}


static void _handleChar(char c) {
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

            recent_sensors[group].group = 0;
            recent_sensors[group].offset = 0;

            _updateSensoryDisplay();

            // TODO: notify the train controller of sensor trigger
        }
    }

    last_byte = (last_byte + 1) % (2 * NUM_SENSORS);
}

static void _sensorTask() {
    while(1) {
        Putc(COM1, SENSOR_QUERY);
        for(int i = 0; i < (2 * NUM_SENSORS); i++) {
            char c = Getc(COM1);
            if(c != 0) {
                _handleChar(c);
                // TODO: check if reading was what we should halt on
            }
        }
    }

}

static void drawSensorArea() {

}

void initSensor() {
    for(int i = 0; i < NUM_RECENT_SENSORS; i++) {
        recent_sensors[i].group = recent_sensors[i].offset = 0;
    }
    for(int i = 0; i < 2 * NUM_SENSORS; i++) {
        sensor_states[i] = 0;
    }
    last_byte = recently_read = 0;
    redrawTrackLayoutGraph('a');
    Putc(COM1, SENSOR_RESET);
    Create(PRIORITY_SENSOR_TASK, _sensorTask);
}

void sensorHalt(int train_number, int sensor, int sensor_number) {
    // gets called by the parser

    int group = (sensor - 'a') * 2;
    if(sensor_number > 8 ) {
        sensor_number -= 8;
        group++;
    }
    int bit = 9 - sensor_number;

    // debug:
    // printf(COM2, "train #%d group %d offset %d\r\n", train_number, group, bit);

    halt_train_number = train_number;
    halt_reading.group = group;
    halt_reading.offset = bit;
}


void redrawTrackLayoutGraph(Track which_track) {
    assert(STR_MAX_LEN > strlen(trackA1));
    assert(STR_MAX_LEN > strlen(trackA2));
    assert(STR_MAX_LEN > strlen(trackA3));
    assert(STR_MAX_LEN > strlen(trackB1));
    assert(STR_MAX_LEN > strlen(trackB2));
    assert(STR_MAX_LEN > strlen(trackB3));

    String s;
    sinit(&s);
    sprintf(&s, "%s%s" VT_CURSOR_HIDE, VT_CURSOR_SAVE);
    vt_pos(&s, VT_TRACK_GRAPH_ROW, VT_TRACK_GRAPH_COL);
    sputstr(&s, VT_RESET);
    PutString(COM2, &s);

    switch(which_track) {
        case A:
            printf(COM2, trackA1);
            printf(COM2, trackA2);
            printf(COM2, trackA3);
            break;
        case B:
            printf(COM2, trackB1);
            printf(COM2, trackB2);
            printf(COM2, trackB3);
            break;
        default:
            break;
    }

    sinit(&s);
    sprintf(&s, "%s%s%s", VT_RESET, VT_CURSOR_RESTORE, VT_CURSOR_SHOW);
    PutString(COM2, &s);
}
