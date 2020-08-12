OUTPUT_FORMAT(binary)

SECTIONS
{
	. = 0x7e00;
	_kstart = .;
	bootstrap : { *(bootstrap) }
	bios : { *(bios) }
	.text : { *(.text) }
	.rodata : { *(.rodata) }
	.data : { *(.data) }
	_kend = .;
	.bss : { *(COMMON) *(.bss) }
	_kbss_end = .;
	KERNEL_SIZE = _kend - _kstart;
}
