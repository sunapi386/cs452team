#ifndef __COMMANDSERVER_H
#define __COMMANDSERVER_H

#define COMMAND_SET_SPEED   30
#define COMMAND_REVERSE     31
#define COMMAND_SET_TURNOUT 32
#define COMMAND_COURIER     33

typedef struct {
    int type;
    int trainNumber;
    int speed;
    int turnoutNumber;
    char turnoutDirection;
} CommandMessage;

void commandServer();

#endif // __COMMANDSERVER_H
