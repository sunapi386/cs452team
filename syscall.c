#include <syscall.h>

#define syscall(SYS_NAME) "swi " #SYS_NAME "\n\t"

int Create(int priority, void (*code) ()) {
    if (priority < 0 || priority >= 32 || code == NULL) {
        return -1; // invalid args
    }
    register int priority_and_return_register asm("r0") = priority;
    register void (*code_register)() asm("r1") = code;
    asm volatile(
        syscall(SYS_CREATE)
        : "+r"(priority_and_return_register)
        : "r"(code_register)
    );
    return priority_and_return_register;
}

int MyTid() {
    register unsigned int return_register asm ("r0");
    asm volatile(
        syscall(SYS_MY_TID)
        : "=r" (return_register)
    );
    return return_register;
}

int MyParentTid() {
    register unsigned int return_register asm ("r0");
    asm volatile(
        syscall(SYS_MY_PARENT_ID)
        : "=r" (return_register)
    );
    return return_register;

}


void Pass() {
    asm volatile(syscall(SYS_PASS));
}

void Exit() {
    asm volatile(syscall(SYS_EXIT));
}

