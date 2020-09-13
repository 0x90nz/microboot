.global do_syscall
tmp: .long 0
do_syscall:
	pushal
	mov 	36(%esp), %eax
	mov 	40(%esp), %ecx
	mov 	44(%esp), %edx
	mov 	48(%esp), %ebx
	mov 	52(%esp), %esi
	mov 	56(%esp), %edi
	int 	$128
	mov	%eax, tmp
	popal
	mov	tmp, %eax
	ret
