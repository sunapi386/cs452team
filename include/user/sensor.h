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
void drawTrackLayoutGraph(Track which_track);

#endif
