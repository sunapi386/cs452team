#include <kernel/context_switch.h>

void KernelExit(struct TaskDescriptor *task) {
    (void)(task);
    asm volatile(
        // # store all kernel registers onto kernel stack
        "stmfd sp!, {r0-r12, lr}\n\t"
        // # change to system mode
        "msr cpsr_c, #0xdf\n\t"
        // # Put sp from TD+16 to sp
        "ldr sp, [r0, #12]\n\t"
        // # Put task->ret to r0
        "ldr r0, [r0, #8]\n\t"
        // # 2) load all user registers
        // # r0: retval r1: pc_usr r2: cpsr_usr\n\t
        "ldmfd   sp!, {r1-r12, lr}\n\t"
        // # change back to supervisor mode
        "msr cpsr_c, #0xd3\n\t"
        // # Put r2 (cpsr_usr) it to spsr_svc
        "msr spsr, r2\n\t"
        // # execute user code
        "movs pc, r1\n\t"
    );
}

void KernelEnter() {
    asm volatile(
        // # Put lr_svc in r1
        "mov r1, lr\n\t"
        // # Put spsr (cpsr_usr) in r2
        "mrs r2, spsr\n\t"
        // # change to system mode
        "msr cpsr_c, #0xdf\n\t"
        // # 2) store all user registers to user stack
        "stmfd   sp!, {r1-r12, lr}\n\t"
        // # put sp_usr in r2
        "mov r1, sp\n\t"
        // # change back to supervisor mode
        "msr cpsr_c, #0xd3\n\t"
        // # now:
        // # r1: sp_usr
        // # load r0 (*task)
        "ldmfd sp!, {r0}\n\t"
        // # store sp in task->sp
        "str r1, [r0, #12]\n\t"
        // # 1) load the rest of the kernel registers from stack
        "ldmfd sp!, {r1-r12, pc}\n\t"
    );
}
void IRQEnter() {
    asm volatile(
        // # go to system mode
        "msr cpsr_c, #0xdf\n\t"
        // # store scratch register on user stack
        "stmfd sp!, {r0-r3}\n\t"
        // # store user sp to r0
        "mov r0, sp\n\t"
        // # make room for user pc and cpsr
        "sub sp, sp, #8\n\t"
        // # go to supervisor mode
        "msr cpsr_c, #0xd3\n\t"
        // # Put lr (pc_usr) to r1, spsr (cpsr_usr) to
        // # r2 and push them onto the user stack
        "mov r1, lr\n\t"
        "mrs r2, spsr\n\t"
        "stmfd r0, {r1, r2}\n\t"
        "bl KernelEnter\n\t"
        // # returned from KernelExit
        // # go to system mode
        "msr cpsr_c, #0xdf\n\t"
        // # restore pc_usr, cpsr_usr
        "ldmfd sp!, {r1, r2}\n\t"
        // # go back to supervisor mode
        "msr cpsr_c, #0xd3\n\t"
        // # restore lr, spsr_svc
        "mov lr, r1\n\t"
        "msr spsr, r2\n\t"
        // # go to system mode
        "msr cpsr_c, #0xdf\n\t"
        // # restore user scratch registers
        "ldmfd sp!, {r0-r3}\n\t"
        // # back to supervisor mode
        "msr cpsr_c, #0xd3\n\t"

        // #mov r0, #1
        // #mov r1, lr
        // #bl bwputr(PLT)

        // # go back to user task
        "subs pc, lr, #4\n\t"
    );
}
