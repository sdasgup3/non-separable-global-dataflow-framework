	.file	"test_2_1.c"
	.section	.rodata
.LC0:
	.string	"%d"
	.text
.globl exmp
	.type	exmp, @function
exmp:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$24, %esp
	movl	$8, -12(%ebp)
	movl	-4(%ebp), %eax
	movl	%eax, -12(%ebp)
	movl	-12(%ebp), %eax
	addl	%eax, -16(%ebp)
	movl	-16(%ebp), %eax
	imull	-12(%ebp), %eax
	movl	%eax, -4(%ebp)
	movl	-16(%ebp), %eax
	sall	$3, %eax
	movl	%eax, -4(%ebp)
	movl	-16(%ebp), %eax
	cmpl	-12(%ebp), %eax
	jge	.L2
	movl	-16(%ebp), %eax
	imull	-12(%ebp), %eax
	movl	%eax, -4(%ebp)
.L2:
	cmpl	$5, -16(%ebp)
	addl	$9, -16(%ebp)
	movl	-16(%ebp), %edx
	movl	-8(%ebp), %eax
	addl	%edx, %eax
	movl	%eax, (%esp)
	call	f
	movl	-8(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	$.LC0, (%esp)
	call	printf
	movl	$0, %eax
	leave
	ret
	.size	exmp, .-exmp
	.comm	x,4,4
	.comm	y,4,4
	.comm	z,4,4
	.ident	"GCC: (GNU) 4.3.0"
	.section	.note.GNU-stack,"",@progbits
