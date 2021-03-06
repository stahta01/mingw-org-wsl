/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 */

	.file	"ilogbf.s"
	.text
	.align 4
.globl _ilogbf
	.def	_ilogbf;	.scl	2;	.type	32;	.endef
_ilogbf:
	flds	4(%esp)
/* I added the following ugly construct because ilogb(+-Inf) is
   required to return INT_MAX in ISO C99.
   -- jakub@redhat.com.  */
	fxam			/* Is NaN or +-Inf?  */
	fstsw   %ax
	movb    $0x45, %dh
	andb    %ah, %dh
	cmpb    $0x05, %dh
	je      1f		/* Is +-Inf, jump.  */

	fxtract
	pushl	%eax
	fstp	%st

	fistpl	(%esp)
	fwait
	popl	%eax

	ret

1:	fstp	%st
	movl	$0x7fffffff, %eax
	ret
