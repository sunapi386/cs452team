#include <user/message_benchmarks.h>
#include <bwio.h>
#include <user/syscall.h>
#include <user/timer.h>

typedef struct Message {
#if USE_4_BYTE_MESSAGE
    int n;
#else
    int n[16];
#endif
} Message;

void sender()
{
    bwprintf(1, "Sender started\n\r");
    bwprintf(1, "Size of message: %d\n\r", sizeof(Message));

    Message msg;
    unsigned int msglen = sizeof(msg);

    // Start 32bit timer
    unsigned int prevTimerVal = 0xffffffff;
    volatile unsigned int *timerVal = (unsigned int *)0;
    InitTimer(&timerVal);

    // Start the benchmark

    int i;
    for (i=0; i<BENCH_ITER; i++)
    {
        Send(3, &msg, msglen, &msg, msglen);
    }

    // Get timer difference
    unsigned int microseconds = (prevTimerVal - *timerVal)/508*1000;
    bwprintf(1, "Finished profiling: %d\n\r", microseconds);

    Exit();
}

void receiver()
{
    bwprintf(1, "Receiver %d started\n\r", MyTid());

    Message msg;
    unsigned int msglen = sizeof(msg);
    int *tid = 0;

    int i;
    for (i=0; i<BENCH_ITER; i++)
    {
        Receive(tid, &msg, msglen);
        //bwprintf(1, "receive returned: %d\n\r", retval);
        Reply(*tid, &msg, msglen);
        //bwprintf(1, "reply returned: %d\n\r", retval);
    }

    Exit();
}

void runBenchmark()
{
#if SEND_FIRST
    Create(2, sender);
    Create(3, receiver);
#else
    Create(3, sender);
    Create(2, receiver);
#endif
    Exit();
}

