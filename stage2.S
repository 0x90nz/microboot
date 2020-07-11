kern_size: .word KERNEL_SIZE

.code16
.global _start
_start:
	# Load our minimal GDT
	lgdt	gdt_desc

	# Set the PE bit
	mov	%cr0, %eax
	or	$1, %eax
	mov	%eax, %cr0

	call	enable_a20

	cli

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
	mov	$0x7ffff, %esp

	call	serial_init

	push	$msg
	call	putstr
	add	$4, %esp

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

serial_init:
	mov	$SER, %dx

	add	$1, %dx		# PORT + 1
	mov	$0x00, %al
	outb	%al, %dx	# Disable interrupts

	add	$2, %dx		# PORT + 3
	mov	$0x80, %al
	outb	%al, %dx	# Enable DLAB

	sub	$3, %dx		# PORT
	mov	$0x0c, %al
	outb	%al, %dx	# Baud rate divisor = 12, for 9600 baud

	add	$1, %dx		# PORT + 1
	mov	$0x00, %al
	outb	%al, %dx	# Divisor high byte

	add	$2, %dx		# PORT + 3
	mov	$0x03, %al
	outb	%al, %dx	# 8N1

	sub	$1, %dx		# PORT + 2
	mov	$0xc7, %al
	outb	%al, %dx	# Enable FIFO with 14 byte thresh

	add	$2, %dx		# PORT + 4
	mov	$0x0b, %al
	outb	%al, %dx	# Enable IRQs

	ret

putc:
	# Get the argument off the stack (char to print)
	mov	4(%esp), %ecx
	mov	$SER, %dx
	add	$5, %dx

	# Loop until the port is ready
1:
	inb	%dx, %al
	and	$0x20, %al
	jz	1b

	# Get back to the actual port addr
	sub	$5, %dx
	
	# Spit out the character
	mov	%cx, %ax
	outb	%al, %dx

	ret

putstr:
	mov	4(%esp), %ecx	# string
	xor	%edx, %edx	# index

1:
	mov	(%ecx,%edx), %eax

	# Check we're not at the null terminator
	test	%al, %al
	jz	2f

	# Save our precious registers
	push	%ecx
	push	%edx

	# arg 1, character
	push	%eax
	call	putc
	add	$4, %esp

	pop	%edx
	pop	%ecx

	inc	%edx
	jmp	1b

2:
	ret


.equ SER, 0x3f8

msg:
.string "Hello World!\n"

.align 8
gdt:
.quad 0x0000000000000000	# Null segment, because we have to
.quad 0x00cf9a000000ffff	# Code segment, 0x08
.quad 0x00cf92000000ffff	# Data segment, 0x10

gdt_desc:
.word gdt_desc - gdt - 1	# Size
.long gdt			# Address

.rept 512
.byte 0x90
.endr
