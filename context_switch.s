	.file	"context_switch.s"
	.text
	.align	2
	.global	KernelExit
	.type	KernelExit, %function
KernelExit:
	@ args = 0, pretend = 0, frame = 12
	@ frame_needed = 1, uses_anonymous_args = 0
	
    # r0 - task, r1 - *request

    # bl firstTask(PLT)

    # 1) store all kernel registers onto kernel stack
    stmfd   sp!, {r0-r12, lr}

    # change to system mode
    msr cpsr_c, #0xdf

    # Put sp from TD to sp
    ldr sp, [r0, #16]

    # 2) load all user registers
    ldmfd   sp!, {r4-r12, lr}

    # change back to supervisor mode
    msr cpsr_c, #0xd3

    # Get cpsr_usr to r0 and put it to spsr_svc
    ldr r3, [r0, #20]
    msr spsr, r3

    mov r0, #1
    mov r1, r4
    bl bwputr(PLT)


    # execute user code
    movs pc, r4

	.size	KernelExit, .-KernelExit
	.align	2
	.global	KernelEnter
	.type	KernelEnter, %function
KernelEnter:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
    
     # Put swi argument in r0
    ldr r0, [lr, #-4]

    # Put lr_svc (pc_usr) in r1
    mov r1, lr

    # Put spsr (cpsr_usr) in r2
    mrs r2, spsr

    # change to system mode
    msr cpsr_c, #0xdf

    # put r1 (pc_usr) in lr_usr
    mov lr, r1

    # 2) store all user registers to user stack
    stmfd   sp!, {r4-r12, pc}
    
    # put sp_usr in r1
    mov r1, sp

    # change back to supervisor mode
    msr cpsr_c, #0xd3

    # now:
    # r0: swi argument
    # r1: sp_usr
    # r2: cpsr_usr

    # 1) load all kernel registers from stack (pick up from where kerexit left off)
    ldmfd sp!, {r0-r12, lr} 
    # replace lr by pc}

	.size	KernelEnter, .-KernelEnter
	.ident	"GCC: (GNU) 4.0.2"
