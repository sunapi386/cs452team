    .file   "context_switch.s"
    .text
    .align  2
    .global kernelExit
    .type   kernelExit, %function
kernelExit:

    # store all kernel registers onto kernel stack
    stmfd sp!, {r0-r12, lr}

    # change to system mode
    msr cpsr_c, #0xdf

    # load task->sp into sp
    ldr sp, [r0, #8]

    # move sp into r1
    mov r1, sp

    # update sp to position after popping user cpsr, pc
    add sp, sp, #8

    # change back to supervisor mode
    msr cpsr_c, #0xd3

    # load user cpsr, user pc -> supervisor lr
    ldmfd r1, {r2, lr}

    # put user cpsr to spsr_svc
    msr spsr, r2

    # change to system mode
    msr cpsr_c, #0xdf

    # load stored user regs from user stack
    ldmfd sp!, {r0-r12, lr}

    # change to supervisor mode
    msr cpsr_c, #0xd3

    # execute user code
    movs pc, lr
    .size   kernelExit, .-kernelExit
    .align  2
    .global irqEnter
    .type   irqEnter, %function
irqEnter:

    # change to system mode
    msr cpsr_c, #0xdf

    # store r1-r12 user registers to user stack
    stmfd sp!, {r0-r12, lr}

    # put user sp in r1
    mov r1, sp

    # calculate user sp after pushing cpsr and pc
    sub sp, sp, #8

    # go to irq mode
    msr cpsr_c, #0xd2

    # put lr-4 (pc_usr) to r3
    sub r3, lr, #4

    # Put spsr (user cpsr) in r2
    mrs r2, spsr

    # change back to supervisor mode
    msr cpsr_c, #0xd3

    # {r1, r2, r3} = {user sp, user cpsr, user pc}, we are svc
    #
    # store r2 (user cpsr), r3 (user pc) to user stack
    stmfd r1!, {r2, r3}

    # load r0 (*task)
    ldmfd sp!, {r0}

    # store r1 (user sp) in task->sp
    str r1, [r0, #8]

    # set syscall ptr to NULL to indicate hwi
    mov r0, #0

    # load the rest of the kernel registers from stack
    ldmfd sp!, {r1-r12, pc}

    .size   irqEnter, .-irqEnter
    .align  2
    .global kernelEnter
    .type   kernelEnter, %function
kernelEnter:

    # r0 - ptr to the request object

    # change to system mode
    msr cpsr_c, #0xdf

    # store all user registers to user stack
    stmfd sp!, {r0-r12, lr}

    # put user sp in r1
    mov r1, sp

    # calculate user sp after pushing cpsr and pc
    sub sp, sp, #8

    # change back to supervisor mode
    msr cpsr_c, #0xd3

    # Put spsr (user cpsr) in r2
    mrs r2, spsr

    # store r2 (user cpsr), lr (user pc) to user stack
    stmfd r1!, {r2, lr}

    # load *task to r2
    ldmfd sp!, {r2}

    # store r1 (user sp) in task->sp
    str r1, [r2, #8]

    # 1) load the rest of the kernel registers from stack
    ldmfd sp!, {r1-r12, pc}

    .size   kernelEnter, .-kernelEnter
    .ident  "GCC: (GNU) 4.0.2"
