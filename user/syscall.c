#include <user/syscall.h>

// Don't modify this! I know there isn't are return statement!
// It's magic! (It needs the request parameter too)
int swi(volatile Syscall *request) {
    (void)request;
    asm volatile("swi");
    register unsigned int r0 asm("r0");
    return r0;
}

int Create(int priority, void (*code) ()) {
    Syscall s;
    s.type = SYS_CREATE;
    s.arg1 = priority;
    s.arg2 = (unsigned int)code;
    return swi(&s);
}

int MyTid() {
    Syscall s;
    s.type = SYS_MY_TID;
    return swi(&s);
}

int MyParentTid() {
    Syscall s;
    s.type = SYS_MY_PARENT_TID;
    return swi(&s);
}

void Pass() {
    Syscall s;
    s.type = SYS_PASS;
    swi(&s);
}

void Exit() {
    Syscall s;
    s.type = SYS_EXIT;
    swi(&s);
}

// #include <debug.h>
// typedef
// struct ClockReq {
//     int type;
//     int data;
// } ClockReq;

// void printBefore(volatile Syscall *s)
// {
//     debug("b4 &syscall: %x", &s);
// }

// void printAfter(volatile Syscall *s)
// {
//     debug("after &syscall: %x", &s);
// }


int Send(int tid, void *msg, unsigned int msglen, void *reply, unsigned int replylen)
{
    Syscall s;

    // printBefore(&s);

    s.type = SYS_SEND;
    s.arg1 = (unsigned int)tid;
    s.arg2 = (unsigned int)msg;
    s.arg3 = (unsigned int)msglen;
    s.arg4 = (unsigned int)reply;
    s.arg5 = (unsigned int)replylen;
    // debug("To: %d, msglen: %d, replylen: %d", s.arg1, s.arg3, s.arg5);
    // ClockReq *req = (ClockReq *)((unsigned int)msg);
    // debug("req->type = %x, req->data = %x", req->type, req->data);

    // printAfter(&s);

    return swi(&s);
}

int Receive(int *tid, void *msg, unsigned int msglen)
{
    Syscall s;
    s.type = SYS_RECEIVE;
    s.arg1 = (unsigned int)tid;
    s.arg2 = (unsigned int)msg;
    s.arg3 = (unsigned int)msglen;
    return swi(&s);
}

int Reply(int tid, void *reply, unsigned int replylen)
{
    Syscall s;
    s.type = SYS_REPLY;
    s.arg1 = (unsigned int)tid;
    s.arg2 = (unsigned int)reply;
    s.arg3 = (unsigned int)replylen;
    return swi(&s);
}

int AwaitEvent(int eventType)
{
    Syscall s;
    s.type = SYS_AWAIT_EVENT;
    s.arg1 = (unsigned int)eventType;
    return swi(&s);
}

void Halt() {
    Syscall s;
    s.type = SYS_HALT;
    swi(&s);
}
