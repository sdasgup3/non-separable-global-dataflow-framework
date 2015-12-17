	.file	"test_3_1.c"
	.section	.rodata
.LC0:
	.string	"%d"
	.text
.globl test
	.type	test, @function
test:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$24, %esp
	movl	$0, -4(%ebp)
.L2:
	cmpl	$2, -4(%ebp)
	jle	.L3
	movl	-12(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	$.LC0, (%esp)
	call	printf
	leave
	ret
.L3:
	cmpl	$1, -4(%ebp)
	jle	.L4
	movl	-8(%ebp), %eax
	movl	%eax, -12(%ebp)
	jmp	.L5
.L4:
	cmpl	$0, -4(%ebp)
	jle	.L6
	movl	-16(%ebp), %eax
	movl	%eax, -8(%ebp)
	jmp	.L5
.L6:
	leal	-16(%ebp), %eax
	movl	%eax, 4(%esp)
	movl	$.LC0, (%esp)
	call	scanf
.L5:
	addl	$1, -4(%ebp)
	jmp	.L2
	.size	test, .-test
	.ident	"GCC: (GNU) 4.3.0"
	.section	.note.GNU-stack,"",@progbits
