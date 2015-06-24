#include <user/user_tasks.h>
#include <bwio.h>
#include <user/syscall.h>
#include <kernel/pl190.h>

void fuked()
{
    bwprintf(COM2, "fuked\n\r");
}

void worker()
{
    register volatile int a = 100;
    register volatile int b = 50;
    register volatile int c = 25;
    register volatile int d = 10;
    register volatile int e = 5;

    bwprintf(COM2, "[worker %d] triggering interrupt!\n\r", MyTid());
    //*(unsigned int *)(VIC1 + VIC_SOFT_INT) = 1;
    //bwprintf(COM2, "[worker %d] back from interrupt!\n\r", MyTid());

    Pass(); // 9

    if (a != 100 || b != 50 || c != 25 || d != 10 || e != 5)
    {
        fuked();
    }
    Exit(); // 6
}

void userTaskHwiTester()
{
    register int a = 0;
    for (a = 0; a < 4; a++)
    {
        bwprintf(COM2, "[userTaskHwiTester] iter %d\n\r", a);
        Create(1, &worker); //5
    }
    bwprintf(COM2, "[userTaskHwiTester] all done... exiting...\n\r");

    Exit(); //9
}

