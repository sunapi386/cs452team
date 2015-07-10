#include <debug.h>
#include <priority.h>
#include <user/syscall.h>
#include <user/nameserver.h>
#include <user/trackserver.h>

void sensorTrigger(char group, int offset) {
    SensorData sd;
    sd.group = group;
    sd.offset = offset;
    //Send();
}

void trackServer() {
    int tid;
    TrackRequest req;
    RegisterAs("trackServer");

    for (;;) {
        // Receive the message
        Receive(&tid, &req, sizeof(req));

        // debug("train controller Received from tid %d", tid);
        // Reply(tid, 0, 0);

        // switch on the type of the message
        switch (req.type) {
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

static int trackServerTid;
void initTrackServer() {
    trackServerTid = Create(PRIORITY_TRACKSERVER, trackServer);
}

void exitTrackServer() {
    // Send(trackServerTid, request, sizeof(TrackR))
    Kill(trackServerTid);
}
