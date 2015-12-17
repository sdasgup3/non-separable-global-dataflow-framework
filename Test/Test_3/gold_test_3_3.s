	.file	"test_3_3.c"
	.section	.rodata
.LC0:
	.string	"dsada"
	.text
.globl test
	.type	test, @function
test:
	pushl	%ebp
	movl	%esp, %ebp
	subl	$24, %esp
	movl	$0, -4(%ebp)
	jmp	.L2
.L6:
	cmpl	$1, -4(%ebp)
	jle	.L3
	movl	-12(%ebp), %eax
	movl	%eax, -16(%ebp)
	jmp	.L4
.L3:
	cmpl	$0, -4(%ebp)
	jle	.L5
	movl	-8(%ebp), %eax
	movl	%eax, -12(%ebp)
	jmp	.L4
.L5:
	movl	$9, -8(%ebp)
.L4:
	addl	$1, -4(%ebp)
.L2:
	cmpl	$2, -4(%ebp)
	jg	.L6
	cmpl	$1, -16(%ebp)
	jle	.L8
	movl	$.LC0, (%esp)
	call	printf
.L8:
	leave
	ret
	.size	test, .-test
	.ident	"GCC: (GNU) 4.3.0"
	.section	.note.GNU-stack,"",@progbits
