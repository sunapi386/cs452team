#include <user/user_tasks.h>
#include <debug.h>
#include <user/syscall.h>
#include <kernel/pl190.h>

void fuked()
{
    debug("fuked");
}

void worker()
{
    register int a = 100;
    register int b = 50;
    register int c = 25;
    register int d = 10;

    *(unsigned int *)(VIC1 + VIC_SOFT_INT) = 1;

    if (a != 100 || b != 50 || c != 25 || d != 10)
    {
        fuked();
    }
    else
    {
        debug("Task %d didn't get fuked and exiting...", MyTid());
    }
    Exit();
}

void userTaskHwiTester()
{
    register int a = 0;
    for (a = 0; a < 4; a++)
    {
        Create(1, &worker);
    }
    debug("Exiting...\n\r");

    Exit();
}

