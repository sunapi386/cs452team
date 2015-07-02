#include <debug.h>
#include <priority.h>
#include <user/syscall.h>
#include <user/nameserver.h>
#include <user/trackserver.h>

void sensorTrigger(char group, int offset)
{
    SensorData sd;
    sd.group = group;
    sd.offset = offset;
    //Send();
}

void trackServer()
{
    int tid;
    //void *data[DATA_BUF_SIZE] = {0, 0, 0, 0}; // 16 byte, 4 ints
    ControllerData data;

    RegisterAs("trackServer");

    for (;;)
    {
        // Receive the message
        Receive(&tid, &data, sizeof(data));

        // debug("train controller Received from tid %d", tid);
        // Reply(tid, 0, 0);

        // switch on the type of the message
        switch (data.type)
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

void initTrackServer() {
    Create(PRIORITY_TRACKSERVER, trackServer);
}
