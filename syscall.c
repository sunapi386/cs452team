#include <syscall.h>

#define str(s) #s
#define xstr(s) str(s)
#define mswi(i)  asm volatile("swi #" xstr(i))
#define swiret(i) mswi(i); \
                  register int ret asm("r0"); \
                  return ret;


int swi(Syscall *request)
{
    asm volatile("swi");
}

int Create(int priority, void (*code) ())
{
    Syscall s;
    s.arg1 = priority;
    s.arg2 = (unsigned int)code;
    
    swiret(SyscallCreate);
}

int MyTid()
{
    Syscall s;
    s.type = SyscallMyTid;
    //bwprintf("Req addr should match this: %x\n\r", &s);
    return swi(&s);
}

int MyParentTid()
{
    Syscall s;
    s.type = SyscallMyParentTid;
    return swi(&s);
}


void Pass()
{
    mswi(SyscallPass);
}

void Exit()
{
    mswi(SyscallExit);
}

