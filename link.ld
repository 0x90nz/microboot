OUTPUT_FORMAT(binary)

SECTIONS
{
	. = 0x7e00;
	_start = .;
	.text : { *(.text) }
	.rodata : { *(.rodata) }
	.data : { *(.data) }
	_end = .;
	.bss : { *(COMMON) *(.bss) }
	KERNEL_SIZE = _end - _start;
}
