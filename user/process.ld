ENTRY(main)
OUTPUT(elf)

SECTIONS
{
    .text : { *(.text) }
    .rodata :  { *(.rodata) }
	.data : { *(.data) }
    exports : { *(exports.start) *(exports) }
	.bss : { *(COMMON) *(.bss) }
}