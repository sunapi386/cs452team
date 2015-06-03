#include <user/clockserver.h>
#include <user/syscall.h>

void clockServerTester()
{
    // start the clock server
    Create(1, &clockServerTask);

    Exit();
}
