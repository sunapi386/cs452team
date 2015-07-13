#include <debug.h>
#include <utils.h>
#include <user/syscall.h>
#include <user/commandserver.h>

void commandServer()
{
    int tid = 0;
    Command cmd;
    for (;;)
    {
        int len = Receive(&tid, &cmd, sizeof(cmd));
        assert(len == sizeof(Command));

        switch (cmd.type) {
        case COMMAND_SET_SPEED:
        {
            int speed = cmd.speed;
            int trainNumber = cmd.trainNumber;


            break;
        }
        case COMMAND_REVERSE:
            break;
        case COMANND_SET_TURNOUT:
            break;
        case COMMAND_SENSOR_REQ:
            break;
        default:
            printf(COM2, "Invalid command\n\r");
            assert(0);
        }
    }
}
