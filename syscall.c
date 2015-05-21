#include <syscall.h>

#define str(s) #s
#define xstr(s) str(s)
#define swi(i)  asm volatile("swi #" xstr(i))

int Create(int priority, void (*code) ())
{
    SyscallArgs s;
    s.arg1 = priority;
    s.arg2 = (unsigned int)code;
    swi(SyscallCreate);
    register int ret asm("r0");
    return ret;
}

int MyTid()
{
    swi(SyscallMyTid);
    register int ret asm("r0");
    return ret;
}

int MyParentTid()
{
    swi(SyscallMyParentTid);
    register int ret asm("r0");
    return ret;
}


void Pass()
{
    swi(SyscallPass);
}

void Exit()
{
    swi(SyscallExit);
}

