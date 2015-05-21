	.file	"context_switch.c"
	.text
	.align	2
	.global	KernelExit
	.type	KernelExit, %function
KernelExit:
	@ args = 0, pretend = 0, frame = 12
	@ frame_needed = 1, uses_anonymous_args = 0
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	sub	sp, sp, #12
	str	r0, [fp, #-20]
	str	r1, [fp, #-24]
	ldr	r3, [fp, #-20]
	ldr	r3, [r3, #20]
	str	r3, [fp, #-16]
	ldr	r3, [fp, #-16]
	mov	r0, #1
	mov	r1, r3
	bl	bwputr(PLT)
	sub	sp, fp, #12
	ldmfd	sp, {fp, sp, pc}
	.size	KernelExit, .-KernelExit
	.align	2
	.global	KernelEnter
	.type	KernelEnter, %function
KernelEnter:
	@ args = 0, pretend = 0, frame = 0
	@ frame_needed = 1, uses_anonymous_args = 0
	mov	ip, sp
	stmfd	sp!, {fp, ip, lr, pc}
	sub	fp, ip, #4
	ldmfd	sp, {fp, sp, pc}
	.size	KernelEnter, .-KernelEnter
	.ident	"GCC: (GNU) 4.0.2"
