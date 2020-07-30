; It might seem odd to mix nasm and gas, but gas is really
; absolute garbage at producing 16 bit code, so here we are.

bits 16
org 0x7c00
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

	push	01h		; num sectors
	push	02h		; start index
	call	read_sector
	add	esp, 2

	; Get the size and figure out how many more sectors
	; we need to load from disk
	mov	ax, [buffer]
	mov 	bx, 512
	div	bx

	; ax now contains file size / 512, which is the number of extra
	; sectors that we need to load

	; skip over loading additional sectors if we don't need to
	test	ax, ax
	jz	xfer

	; While it's not really efficient, it's easy. We just take the performance hit
	; that comes from reloading the first sector

	; Add 1 to ax so we get _all_ the sectors we need to load
	add	ax, 1

	push	ax		; num sectors
	push	02h		; start index
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
	mov	al, [esp + 20]
	mov	cx, [esp + 18]
	mov	ah, 02h		; function code
	mov	dl, [disk_num]	; disk number
	mov	dh, 00h		; head number
	mov	bx, buffer

	int	13h

	popa
	ret


; Disk stuff
buffer equ 07e00h
kern_start equ 07e02h
disk_num: db 0

msg db `Loading binary\r\n`, 0
verinfo db `\xe6boot v0.1.0\r\n`,0
s2msg db `Entering second stage\r\n`, 0

; Boot signature and padding
times	510-($-$$) db 0
dw 0xaa55