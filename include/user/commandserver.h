#ifndef __COMMANDSERVER_H
#define __COMMANDSERVER_H

#define COMMAND_SET_SPEED   30
#define COMMAND_REVERSE     31
#define COMANND_SET_TURNOUT 32
#define COMMAND_SENSOR_REQ  33

typedef struct {
    int type;
    int speed;
    int trainNumber;
    int turnoutNumber;
    char turnoutDirection;
} Command;

void commandServer();

#endif // __COMMANDSERVER_H
