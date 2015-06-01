#define SYSCALL_DEFNS
#include <syscall.h>
#undef SYSCALL_DEFNS

static Syscall s;

Syscall *initSyscall()
{
    s.type = 0;
    return &s;
}

// Don't modify this! I know there isn't are return statement!
// It's magic! (It needs the request parameter too)
int swi(Syscall *request) {
    (void)request;
    asm volatile("swi");
    register unsigned int r0 asm("r0");
    return r0;
}

int Create(int priority, void (*code) ()) {
    s.type = SYS_CREATE;
    s.arg1 = priority;
    s.arg2 = (unsigned int)code;
    return swi(&s);
}

int MyTid() {
    s.type = SYS_MY_TID;
    return swi(&s);
}

int MyParentTid() {
    s.type = SYS_MY_PARENT_TID;
    return swi(&s);
}


void Pass() {
    s.type = SYS_PASS;
    swi(&s);
}

void Exit() {
    s.type = SYS_EXIT;
    swi(&s);
}

int Send(int tid, void *msg, unsigned int msglen, void *reply, unsigned int replylen)
{
    s.type = SYS_SEND;
    s.arg1 = (unsigned int)tid;
    s.arg2 = (unsigned int)msg;
    s.arg3 = (unsigned int)msglen;
    s.arg4 = (unsigned int)reply;
    s.arg5 = (unsigned int)replylen;
    return swi(&s);
}

int Receive(int *tid, void *msg, unsigned int msglen)
{
    s.type = SYS_RECEIVE;
    s.arg1 = (unsigned int)tid;
    s.arg2 = (unsigned int)msg;
    s.arg3 = (unsigned int)msglen;
    return swi(&s);
}

int Reply(int tid, void *reply, unsigned int replylen)
{
    s.type = SYS_REPLY;
    s.arg1 = (unsigned int)tid;
    s.arg2 = (unsigned int)reply;
    s.arg3 = (unsigned int)replylen;
    return swi(&s);
}

