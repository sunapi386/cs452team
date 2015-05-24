#include <syscall.h>

static Syscall s;

// Don't modify this! I know there isn't a return statement!
// It's magic! (It needs the request parameter too)
static inline int swi(Syscall *request) {
    asm volatile("swi");
}

int create(int priority, void (*code) ()) {
    s.type = SYS_CREATE;
    s.arg1 = priority;
    s.arg2 = (unsigned int)code;
    return swi(&s);
}

int myTid() {
    s.type = SYS_MY_TID;
    return swi(&s);
}

int myParentTid() {
    s.type = SYS_MY_PARENT_TID;
    return swi(&s);
}


void pass() {
    s.type = SYS_PASS;
    swi(&s);
}

void exit() {
    s.type = SYS_EXIT;
    swi(&s);
}
