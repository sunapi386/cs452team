#ifndef __SENSOR_H
#define __SENSOR_H

/**
Sensor receives sensor data from tracks and handles drawning on the screen.
Also handles sensor manipulations, i.e. stopping a train when it has reached
a certain sensor.
*/

void initSensor();
// e.g. sensorHalt(42, 'a', 3);
void sensorHalt(int train_number, int sensor, int sensor_number);
typedef enum {A, B, None} Track;
void redrawTrackLayoutGraph(Track which_track);

// putting the tracks here to not pollute .c file

static char *trackA1 =
"=========S===S===================================\\\r\n"
"=======S/   /  ==========S========S============\\  \\\r\n"
"======/    S==/           \\   |  /              \\  \\\r\n"
"          /                \\  |S/                \\==S\r\n";

static char *trackA2 =
"         |                  \\S|                     |\r\n"
"         |                    |S\\                   |\r\n"
"          \\                 /S|  \\               /==S\r\n"
"           S==\\            /  |   \\             /  /\r\n";

static char *trackA3 =
"========\\   \\  ==========S=========S===========/  /\r\n"
"=========S\\  \\=========S============S============/\r\n"
"===========S\\           \\          /\r\n"
"=============S===========S========S============\r\n";


static char *trackB1 =
"=========S===S===================================\\\r\n"
"=======S/   /  ==========S========S============\\  \\\r\n"
"      /    S==/           \\   |  /              \\  \\\r\n"
"   /=/    /                \\  |S/                \\==S\r\n";

static char *trackB2 =
"   |     |                  \\S|                     |\r\n"
"   |     |                    |S\\                   |\r\n"
"   \\=     \\                 /S|  \\               /==S\r\n"
"     \\=\\   S==\\            /  |   \\             /  /\r\n";

static char *trackB3 =
"        \\   \\  ==========S=========S===========/  /\r\n"
"=========S\\  \\=========S============S============/\r\n"
"===========S\\           \\          /\r\n"
"=============S===========S========S============\r\n";

#endif
