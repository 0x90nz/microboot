OUTPUT_FORMAT(binary)

SECTIONS
{
	. = 0x7e00;
	_start = .;
	.text : { *(.text) }
	.rodata : { *(.rodata) }
	.data : { *(.data) }
	.bss : { *(COMMON) *(.bss) }
	_end = .;
	KERNEL_SIZE = _end - _start;
}
