.word KERNEL_SIZE
jmp	_start

.align 16
.code16
.global _start
_start:
	# Store the memory info we got from stage1 somewhere
	# we can get to it easier
	pop	%ax
	mov	%ax, memory_config + 8

	pop	%ax
	mov	%ax, memory_config
	pop	%ax
	mov	%ax, memory_config + 2
	pop	%ax
	mov	%ax, memory_config + 4
	pop	%ax
	mov	%ax, memory_config + 6

	# Load our minimal GDT
	lgdt	gdt_desc

	# Set the PE bit
	mov	%cr0, %eax
	or	$1, %eax
	mov	%eax, %cr0

	call	enable_a20

	# Perform a far jump
	data32 ljmp	$0x08, $1f
.code32
1:
	mov	$0x10, %ax
	mov	%ax, %ds
	mov	%ax, %es
	mov	%ax, %fs
	mov 	%ax, %gs
	mov	%ax, %ss

	# Setup the stack
	mov	$0x70000, %esp

	push	$memory_config
	call	kernel_main

	# We should never get here. If we do, just hang
	cli	
1:
	hlt
	jmp	1b


enable_a20:
1:
	inb	$0x64, %al
	testb	$0x02, %al
	jnz	1b

	movb	$0xd1, %al
	outb	%al, $0x64

1:
	inb	$0x64, %al
	testb	$0x02, %al
	jnz	1b

	movb	$0xdf, %al
	outb	%al, $0x60

1:
	inb	$0x64, %al
	testb	$0x02, %al
	jnz 1b
	ret

.align 8
gdt:
.quad 0x0000000000000000	# Null segment, because we have to
.quad 0x00cf9a000000ffff	# Code segment, 0x08
.quad 0x00cf92000000ffff	# Data segment, 0x10

gdt_desc:
.word gdt_desc - gdt - 1	# Size
.long gdt			# Address

.global memory_config
memory_config:
.rept 5
.word 0
.endr
