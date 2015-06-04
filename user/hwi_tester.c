#include <user/user_tasks.h>
#include <bwio.h>
#include <user/syscall.h>
#include <kernel/pl190.h>

static void grunt()
{
    bwprintf(COM2, "[grunt %d] triggering interrupt!\n\r", MyTid());
    *(unsigned int *)(VIC1 + VIC_SOFT_INT) = 1;
    bwprintf(COM2, "[grunt %d] back from interrupt!\n\r", MyTid());
    Exit();
}

void userTaskHwiTester()
{
    register int a = 0;
    for (a = 0; a < 4; a++)
    {
        bwprintf(COM2, "[userTaskHwiTester] iter %d\n\r", a);
        Create(1, &grunt);
    }
    bwprintf(COM2, "[userTaskHwiTester] all done... exiting...\n\r");

    Exit();
}

