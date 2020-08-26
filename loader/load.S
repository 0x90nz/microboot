; It might seem odd to mix nasm and gas, but gas is really
; absolute garbage at producing 16 bit code, so here we are.

bits 16
org 0x7c00
jmp	0:_start
_start:
	mov	ax, 0
	mov	ds, ax
	mov	es, ax
	mov	fs, ax
	mov	gs, ax

	mov	[disk_num], dl
	
	; Set sp to be 7000:f000 (which is the end of conventional memory)
	; so it doesn't mess with our stuff We disable interrupts while doing 
	; this because apparently they can mess with it in a bad way.
	cli
	mov	ax, 0x7000
	mov	ss, ax
	mov	sp, 0xf000
	sti

	push	verinfo
	call	puts
	add	esp, 2

	push	msg
	call	puts
	add	esp, 2

	push	01h			; num sectors
	push	WORD [kimage_start]	; start index
	call	read_sector
	add	esp, 4

	; Get the size and figure out how many more sectors
	; we need to load from disk
	mov	ax, WORD [buffer]

	mov	dx, 0
	mov 	bx, 512

	div	bx

	; ax now contains file size / 512, which is the number of extra
	; sectors that we need to load

	; skip over loading additional sectors if we don't need to
	test	ax, ax
	jz	xfer

	add	ax, 1

	push	ax			; num sectors
	push	WORD [kimage_start]	; start index
	call	read_sector
	add	esp, 4

xfer:
	push	s2msg
	call	puts
	add	esp, 2

	; Get memory info to use later
	mov	ax, 0e801h
	int	15h

	push	dx	; configured 2
	push	cx	; configured 1
	push	bx	; extended 2
	push 	ax	; extended 1

	xor	ax, ax
	mov	al, [disk_num]
	push	ax	; disk number

	cli
	jmp 	kern_start

	; If for some reason we get back here, just hang
	cli
loop:
	hlt
	jmp	loop

puts:
	push	bx
	mov	bx, [esp + 4]

puts_loop:
	mov	al, [bx]
	test	al, al
	jz	puts_done

	push	ax
	call	putc
	add	esp, 2 
	inc	bx
	jmp	puts_loop

puts_done:
	pop	bx
	ret

putc:
	push	bx
	mov	al, [esp + 4]	; char to print
	mov	ah, 0eh		; function (write text)
	mov	bx, 0		; zero out page number
	int	10h
	pop	bx
	ret

; Read a n sectors
read_sector:
	pusha
	mov	ax, [esp + 20]
	mov	[dsk_num_sec], ax

	mov	ax, [esp + 18]
	mov	[dsk_lo_lba], ax

	mov	DWORD [dsk_buf], buffer
	mov	dl, [disk_num]

	mov	si, dsk_addr
	mov	ax, 4200h

	int	13h

	popa
	ret


; Disk stuff
buffer equ 07e00h
kern_start equ 07e02h
disk_num: db 0

; The disk address packet structure that is required for
; int 13h ah=42h
dsk_addr:
db 10h
db 0
dsk_num_sec dw 0
dsk_buf dd 0
dsk_lo_lba dd 0
dsk_hi_lba dd 0

; The starting **logical block** of the kernel on disk
kimage_start dw 1

msg db `Loading binary\r\n`, 0
verinfo db `\xe6boot v0.1.0\r\n`,0
s2msg db `Entering second stage\r\n`, 0
