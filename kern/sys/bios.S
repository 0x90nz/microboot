saved_if: .byte 0

saved_gdt_ptr:
.short 0
.long 0

saved_idt_ptr:
.short 0
.long 0

realmode_idt_ptr:
.short 0x3ff
.long 0

realmode_gdt_ptr:
.short 0
.long 0

pic1_saved_mask: .byte 0
pic2_saved_mask: .byte 0

# Is this really needed if we don't point to it?
.global realmode_gdt
realmode_gdt:
.rept 10
.long 0
.long 0
.endr

saved_esp: .long 0
saved_ebp: .long 0
saved_eax: .long 0
saved_ds: .word 0
saved_es: .word 0
saved_fl: .word 0

.global bios_regs
bios_regs:
.rept 42
.byte 0
.endr

.global _internal_bios_interrupt
_internal_bios_interrupt:
	push	%ebp
	mov	%esp, %ebp
	pushal

	# Save the interrupt flag, so it can be restored later
	mov	%al, saved_if
	cli

	# Save the GDT and IDT so we can get them back later
	sgdt	(saved_gdt_ptr)
	sidt	(saved_idt_ptr)

	# Eventually we'll want to allow for a GDT which might be outside
	# of low memory, for now it works without that
	# lgdt	(realmode_gdt_ptr)
	lidt	(realmode_idt_ptr)

	# When we go into real mode, it'll pess with the PICs, so we save the
	# masks now so that we can get them back later
	pushl	$0
	call	get_pic_mask
	add	$4, %esp
	mov	%al, pic1_saved_mask

	pushl	$1
	call	get_pic_mask
	add	$4, %esp
	mov	%al, pic1_saved_mask

	mov	$intr, %ebx
	movb	8(%ebp), %al	# Get the argument off the stack
	movb	%al, 1(%ebx)	# Patch the instruction with the interrupt number

	# We need to use a long jump to load %cs to our 16 bit code segment
	ljmp	$0x18,$0f	
0:
.code16
	# Disable protected mode
	mov	%cr0, %eax
	and	0xfffe, %ax
	mov	%eax, %cr0

	# Load %cs to 0
	ljmp	$0,$0f
0:
	xor	%ax, %ax
	mov 	%ax, %ds
	mov 	%ax, %es
	mov 	%ax, %ss
	mov 	%ax, %fs
	mov 	%ax, %gs

	mov	%esp, saved_esp
	mov	%ebp, saved_ebp
	movl	$(bios_regs), %esp

	popal
	popfw
	pop	%es
	pop	%ds

	mov	$0xf000, %esp

intr:	int	$0
	cli			# Some BIOS' enable interrupts, don't let 'em do that
	
	mov	%eax, saved_eax
	mov	%ds, saved_ds
	mov	%es, saved_es

	pushfw
	popw	%ax
	mov	%ax, saved_fl

	mov	%cr0, %eax
	or	$1, %ax
	mov	%eax, %cr0

	ljmp	$0x08,$0f
0:	
.code32
	mov	$0x10, %ax
	mov 	%ax, %ds
	mov 	%ax, %es
	mov 	%ax, %ss
	mov 	%ax, %fs
	mov 	%ax, %gs

	# lgdt	(saved_gdt_ptr)
	lidt	(saved_idt_ptr)

	mov	saved_ebp, %ebp
	movl	$(bios_regs), %esp
	add	$38, %esp

	mov	saved_ds, %ax
	pushw	%ax
	mov	saved_es, %ax
	pushw	%ax

	pushfw
	popw	%ax
	and	$0xfffe, %ax
	or	saved_fl, %ax
	pushw	%ax

	mov	saved_eax, %eax
	pushal
	mov	saved_esp, %esp

	call	interrupts_pic_init

	movzbl	pic1_saved_mask, %eax
	push	%eax
	pushl	$0
	call	set_pic_mask
	add	$8, %esp

	movzbl	pic2_saved_mask, %eax
	push	%eax
	pushl	$1
	call	set_pic_mask
	add	$8, %esp

	in	$0x60, %al

	movzbl	saved_if, %eax
	pushl	%eax
	call	set_if
	add	$4, %esp

	popal
	pop	%ebp
	ret

get_if:
	pushf
	popl	%eax
	shr	$9, %eax	# flags >> 9
	and	$1, %eax	# flags & 1, mask the interrupt bit
	ret

set_if:
	cmpl	$0, 4(%esp)	# if 1, sti else cli
	jz	0f
	sti
	ret
0:
	cli
	ret