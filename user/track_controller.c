#include <debug.h>
#include <priority.h>
#include <user/syscall.h>
#include <user/nameserver.h>
#include <user/track_controller.h>

#define DATA_BUF_SIZE 4

void sensorTrigger(char group, int offset)
{
    SensorData sd;
    sd.group = group;
    sd.offset = offset;
    //Send();
}

void trackControllerTask()
{
    int tid;
    void *data[DATA_BUF_SIZE] = {0, 0, 0, 0}; // 16 byte, 4 ints

    RegisterAs("controller");

    for (;;)
    {
        // Receive the message
        Receive(&tid, data, sizeof(data));
        ControllerData *ctrlData = (ControllerData *)data;
        // debug("train controller Received from tid %d", tid);
        // Reply(tid, 0, 0);

        // switch on the type of the message
        switch (ctrlData->type)
        {
        case type_sensor:
            break;
        case type_turnout:
            break;
        default:
            break;
        }
    }
    Exit();
}

void initController() {
    Create(PRIORITY_TRACK_CONTROLLER_TASK, trackControllerTask);
}
