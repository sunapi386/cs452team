#include <string.h>
#include <priority.h>
#include <user/vt100.h>
#include <user/sensor.h>
#include <user/syscall.h>

/* Each sensor stores 2 bytes.
*  To read sensor they must be told to dump their memory.
*  Two options for requesting this dump: read one, read many.
*  In read one, send 192 + num_of_unit_to_read
*  In read many, 128 + num_of_last_unit_to_read
*  The trains lab has 5 sensor modules.
*/

#define NUM_SENSORS     5
#define RESET           192
#define NORESET         128
#define QUERY           (NORESET + NUM_SENSORS)

struct Sensor {
    char byte;
    char bit;
} Sensor;


// try to draw the as litte as possible by only what is changed (difference)
static inline void updateSensorPanel(struct Sensor *sensors, int index) {
    String s;
    sinit(&s);
    vt_pos(&s, VT_SENSOR_ROW, VT_SENSOR_COL);

    for(int i = 0; i < NUM_SENSORS; i++) {
        sprintf(&s, "[%c,%c]", sensors[i].byte, sensors[i].bit);
    }
    // PutString(COM2, &s);
}

static void sensorTask() {
    struct Sensor sensors[NUM_SENSORS];
    for(int i = 0; i < NUM_SENSORS; i++) {
        sensors[i].byte = sensors[i].bit = 0;
    }
    int last_read = 0;

    // setup COM1 for reading
    String query;
    sinit(&query);
    sprintf(&query, "%c%c", RESET, QUERY);
    // PutString(COM1, &query);

    while (1) {
        // Request for sensor data
        char c = Getc(COM1);

        // Get 10 sensor updates

        last_read = (last_read + 1) % 10;
    }
}

void sensorInit() {
    Create(PRIORITY_SENSOR_TASK, sensorTask);
}

void sensorQuit() {

}

