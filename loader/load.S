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

	; init some values we'll need for the DAP while we still have zero in ax
	mov	[dsk_pkt_sz], ax
	mov	[dsk_pkt_zero], ax

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
	add	sp, 2

	push	loading_msg
	call	puts
	add	sp, 2

	push    0x07e0
	push    0
	push	01h		; num sectors
	push	kimage_start	; start index
	call	read_sector
	add	sp, 8

	; Get the size and figure out how many more sectors
	; we need to load from disk
	mov	ax, WORD [buffer]
	mov     dx, WORD [buffer + 2]

	mov 	bx, 512
	div	bx

	; ax now contains file size / 512, which is the number of extra
	; sectors that we need to load

	; Some BIOS' are picky about reading more than 64k at once so here we
	; limit the number of sectors that we copy to 64k and then copy 64k
	; chunks until we're done

	; skip over loading additional sectors if we don't need to
	test	ax, ax
	jz	.xfer

	; if there's no remainder the image was perfectly aligned,
	; so no need to read an additional sector
	test    dx, dx
	jz      .read_sectors

	add	ax, 1			; number of sectors left
	mov     bx, kimage_start	; current sector number
	mov     cx, 0x07e0		; sector, offset is always 0

.read_sectors:
	cmp     ax, 0x80
	jl      .read_last

	push    cx	; buffer address (seg then off)
	push    0
	push    0x80	; number of sectors (64k)
	push    bx	; start sector
	call    read_sector
	add     sp, 8

	add     bx, 0x80
	add     cx, 0x1000
	sub     ax, 0x80
	jmp     .read_sectors

.read_last:
	push    cx
	push    0
	push	ax	; num sectors
	push	bx	; start index
	call	read_sector
	add	sp, 8

.xfer:
	push	s2msg
	call	puts
	add	sp, 2

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

	jmp 	kern_start

	; If for some reason we get back here, just hang
	cli

halt_forever:
	hlt
	jmp	halt_forever

print_hex_word:
	push    bp
	mov     bp, sp
	push    bx
	push    cx

	mov     bx, [bp + 4]
	mov     cx, 12
.loop:
	mov     ax, bx
	shr     ax, cl
	and     ax, 0x0f

	cmp     ax, 0x0a
	jl      .nr_offset
	; add 39 to ax so that when we add '0' to it we'll be in the alpha
	; range instead of the numeric range
	add     ax, 39
.nr_offset:
	add     ax, '0'
	push    ax
	call    putc

	sub     cx, 4
	jns     .loop

	pop     cx
	pop     bx
	mov     sp, bp
	pop     bp
	ret

puts:
	push    bp
	mov     bp, sp
	push	bx

	mov	bx, [bp + 4]

puts_loop:
	mov	al, [bx]
	test	al, al
	jz	puts_done

	push	ax
	call	putc
	add	sp, 2
	inc	bx
	jmp	puts_loop

puts_done:
	pop     bx
	mov     sp, bp
	pop     bp
	ret

putc:
	push    bp
	mov     bp, sp
	push	bx

	mov	al, [bp + 4]	; char to print
	mov	ah, 0eh		; function (write text)
	mov	bx, 0		; zero out page number
	int	10h

	pop     bx
	mov     sp, bp
	pop     bp
	ret

; Read a n sectors
read_sector:
	push    bp
	mov     bp, sp
	pusha

	; Address (seg:off) to store at
	mov     ax, [bp + 10]
	mov     WORD [dsk_buf + 2], ax
	mov     ax, [bp + 8]
	mov     WORD [dsk_buf], ax

	mov	ax, [bp + 6]
	mov	[dsk_num_sec], ax

	mov	ax, [bp + 4]
	mov	[dsk_lo_lba], ax

	mov	dl, [disk_num]

	mov	si, dsk_addr
	mov	ax, 4200h

	int	13h

	jc      disk_error

	popa
	mov     sp, bp
	pop     bp
	ret

disk_error:
	push    ax

	push	disk_error_msg
	call	puts
	add	sp, 2

	pop     ax
	shr     ax, 8
	push    ax
	call    print_hex_word
	add     sp, 2

	jmp	halt_forever

; Disk stuff
buffer		equ 07e00h
kern_start	equ 07e04h
disk_num:
	db 0

; The disk address packet structure that is required for
; int 13h ah=42h
dsk_addr 	equ 0x500
dsk_pkt_sz	equ dsk_addr + 0
dsk_pkt_zero	equ dsk_addr + 1
dsk_num_sec 	equ dsk_addr + 2
dsk_buf 	equ dsk_addr + 4
dsk_lo_lba 	equ dsk_addr + 8
dsk_hi_lba 	equ dsk_addr + 10

; The starting **logical block** of the kernel on disk
kimage_start	equ 1

loading_msg	db `Loading binary\r\n`, 0
verinfo		db `\xe6boot v0.1.0\r\n`,0
s2msg		db `Entering second stage\r\n`, 0
disk_error_msg	db `Disk read error, error code: `, 0
