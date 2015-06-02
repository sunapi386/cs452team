#include <user/clockserver.h>
#include <user/syscall.h>

#define TIME        0
#define DELAY       1
#define DELAY_UNTIL 2

typedef
struct ClockReq {
    int type;
    int ticks;
} ClockReq;

int Time()
{
    ClockReq req;
    req.type = TIME;
}

int Delay(int ticks)
{
    ClockReq req;
    req.type = DELAY;
    req.ticks = ticks;
}

int DelayUntil(int ticks)
{
    ClockReq req;
    req.type = DELAY_UNTIL;
    req.ticks = ticks;


}

void clockServer()
{
    int tid = 0;
    ClockReq req;
    unsigned int reqLen = sizeof(req);

    for (;;)
    {
        Receive(&tid, &req, reqLen);
        switch (req.type)
        {
        case TIME:

        case DELAY:
        case DELAY_UNTIL:
        default:
            break;
        }
    }
}

void clockNotifier()
{

}
