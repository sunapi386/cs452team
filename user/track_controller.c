#include <user/syscall.h>
#include <user/track_controller.h>

void trackController()
{
    int tid;
    void *data[4] = {0, 0, 0, 0}; // 16 byte, 4 ints

    for (;;)
    {
        // Receive the message
        Receive(&tid, data, sizeof(data));
        ControllerData *ctrlData = (ControllerData *)data;

        // switch on the type of the message
        switch (ctrlData->tag)
        {
        case tag_sensor:
            break;
        default:
            break;
        }
    }
    Exit();
}
