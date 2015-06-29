#include <user/syscall.h>
#include <user/track_controller.h>

void trackController()
{
    int tid;
    void *data[4]; // 16 byte, 4 ints

    for (;;)
    {
        // Receive the message
        Receive(&tid, data, sizeof(data));

        // switch on the type of the message
    }
    Exit();
}
