#include <syscall.h>

#define str(s) #s
#define xstr(s) str(s)
#define swi(i)  asm volatile("swi #" xstr(i))
#define swiret(i) swi(i); \
                  register int ret asm("r0"); \
                  return ret;

int Create(int priority, void (*code) ())
{
    SyscallArgs s;
    s.arg1 = priority;
    s.arg2 = (unsigned int)code;
    
    swiret(SyscallCreate);
}

int MyTid()
{
    swiret(SyscallMyTid);
}

int MyParentTid()
{
    swiret(SyscallMyParentTid);
}


void Pass()
{
    swi(SyscallPass);
}

void Exit()
{
    swi(SyscallExit);
}

