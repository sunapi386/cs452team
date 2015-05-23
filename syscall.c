#include <syscall.h>

static Syscall s;

// Don't modify this! I know there isn't are return statement!
// It's magic! (It needs the request parameter too)
int swi(Syscall *request)
{
    asm volatile("swi");
}

int Create(int priority, void (*code) ())
{
    s.type = SYS_CREATE:
    s.arg1 = priority;
    s.arg2 = (unsigned int)code;
    return swi(&s);
}

int MyTid()
{
    s.type = SYS_MY_TID;
    return swi(&s);
}

int MyParentTid()
{
    s.type = SYS_MY_PARENT_TID;
    return swi(&s);
}


void Pass()
{
    s.type = SYS_PASS;
    swi(&s);
}

void Exit()
{
    s.type = SYS_EXIT;
    swi(&s);
}
