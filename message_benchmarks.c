#include <bwio.h>
#include <syscall.h>

void sender()
{
    bwprintf(1, "Sender started\n\r");
    Exit();
}

void receiver()
{
    bwprintf(1, "Receiver started\n\r");
    Exit();
}

void runBenchmark()
{
#if SEND_FIRST
    Create(1, sender);
    Create(2, receiver);
#else
    Create(2, receiver);
    Create(1, sender);
#endif
}
