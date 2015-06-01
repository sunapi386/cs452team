    .file   "context_switch.s"
    .text
    .align  2
    .global KernelExit
    .type   KernelExit, %function
KernelExit:
    @ args = 0, pretend = 0, frame = 12
    @ frame_needed = 1, uses_anonymous_args = 0

    # store all kernel registers onto kernel stack
    stmfd sp!, {r0-r12, lr}

    # change to system mode
    msr cpsr_c, #0xdf

    # Put sp from TD+16 to sp
    ldr sp, [r0, #12]

    # Put task->ret to r0
    ldr r0, [r0, #8]

    # 2) load all user registers
    # r0: retval r1: pc_usr r2: cpsr_usr
    ldmfd   sp!, {r1-r12, lr}

    # change back to supervisor mode
    msr cpsr_c, #0xd3

    # Put r2 (cpsr_usr) it to spsr_svc
    msr spsr, r2

    # execute user code
    movs pc, r1
    .size   KernelExit, .-KernelExit
    .align  2
    .global IRQEnter
    .type   IRQEnter, %function
IRQEnter:

    # go to system mode
    msr cpsr_c, #0xdf

    # store scratch register on user stack
    stmfd sp!, {r0-r3}

    # store user sp to r0
    mov r0, sp

    # make room for user pc and cpsr
    sub sp, sp, #8

    # go back to IRQ mode
    msr cpsr_c, #0xd2

    # Put lr (pc_usr) to r1, spsr (cpsr_usr) to
    # r2 and push them onto the user stack
    mov r1, lr
    mrs r2, spsr
    stmfd r0, {r1, r2}

    # go to supervisor mode
    # msr cpsr_c, #0xd3

    bl KernelEnter
    # returned from KernelExit

    # go to system mode
    msr cpsr_c, #0xdf

    # restore pc_usr, cpsr_usr
    ldmfd sp!, {r1, r2}

    # go back to IRQ mode
    msr cpsr_c, #0xd2

    # restore lr, spsr_svc
    mov lr, r1
    msr spsr, r2

    # go to system mode
    msr cpsr_c, #0xdf

    # restore user scratch registers
    ldmfd sp!, {r0-r3}

    # back to IRQ mode
    msr cpsr_c, #0xd2

    # go back to user task
    subs pc, lr, #4
    .size   IRQEnter, .-IRQEnter
    .align  2
    .global KernelEnter
    .type   KernelEnter, %function
KernelEnter:
    @ args = 0, pretend = 0, frame = 0
    @ frame_needed = 1, uses_anonymous_args = 0

    # Put lr_svc in r1
    mov r1, lr

    # Put spsr (cpsr_usr) in r2
    mrs r2, spsr

    # change to system mode
    msr cpsr_c, #0xdf

    # 2) store all user registers to user stack
    stmfd   sp!, {r1-r12, lr}

    # put sp_usr in r2
    mov r1, sp

    # change back to supervisor mode
    msr cpsr_c, #0xd3

    # now:
    # r1: sp_usr

    # load r0 (*task)
    ldmfd sp!, {r0}

    # store sp in task->sp
    str r1, [r0, #12]

    # 1) load the rest of the kernel registers from stack
    ldmfd sp!, {r1-r12, pc}

    .size   KernelEnter, .-KernelEnter
    .ident  "GCC: (GNU) 4.0.2"
