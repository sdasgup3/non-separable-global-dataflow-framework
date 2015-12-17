	.file	"test_1.c"
	.text
.globl exmp
	.type	exmp, @function
exmp:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$24, %esp
	movl	$4, -12(%ebp)
	movl	-8(%ebp), %edx
	movl	-12(%ebp), %eax
	addl	%edx, %eax
	movl	%eax, -16(%ebp)
	movl	-16(%ebp), %eax
	imull	-12(%ebp), %eax
	movl	%eax, -4(%ebp)
	movl	x, %edx
	movl	y, %eax
	cmpl	%eax, %edx
	jge	.L2
	movl	-8(%ebp), %eax
	movl	-16(%ebp), %edx
	movl	%edx, %ecx
	subl	%eax, %ecx
	movl	%ecx, %eax
	movl	%eax, -12(%ebp)
	jmp	.L3
.L2:
	movl	-12(%ebp), %eax
	addl	%eax, -8(%ebp)
	movl	y, %edx
	movl	x, %eax
	cmpl	%eax, %edx
	jle	.L4
.L5:
	movl	-12(%ebp), %edx
	movl	-16(%ebp), %eax
	addl	%edx, %eax
	movl	%eax, -4(%ebp)
	movl	-8(%ebp), %edx
	movl	-12(%ebp), %eax
	addl	%edx, %eax
	movl	%eax, (%esp)
	call	f
	movl	y, %edx
	movl	x, %eax
	cmpl	%eax, %edx
	jg	.L5
	jmp	.L6
.L4:
	movl	-16(%ebp), %eax
	imull	-12(%ebp), %eax
	movl	%eax, -8(%ebp)
	movl	-12(%ebp), %edx
	movl	-16(%ebp), %eax
	subl	%edx, %eax
	movl	%eax, (%esp)
	call	f
.L6:
	movl	-12(%ebp), %edx
	movl	-16(%ebp), %eax
	addl	%edx, %eax
	movl	%eax, (%esp)
	call	g
	movl	z, %edx
	movl	x, %eax
	cmpl	%eax, %edx
	jg	.L2
.L3:
	movl	-8(%ebp), %edx
	movl	-16(%ebp), %eax
	subl	%edx, %eax
	movl	%eax, (%esp)
	call	h
	movl	-8(%ebp), %edx
	movl	-12(%ebp), %eax
	addl	%edx, %eax
	movl	%eax, (%esp)
	call	f
	leave
	ret
	.size	exmp, .-exmp
	.comm	x,4,4
	.comm	y,4,4
	.comm	z,4,4
	.ident	"GCC: (GNU) 4.3.0"
	.section	.note.GNU-stack,"",@progbits
