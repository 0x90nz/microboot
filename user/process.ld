ENTRY(main)
OUTPUT(elf)

SECTIONS
{
    .text : { *(.text) }
    .rodata :  { *(.rodata) }
	.data : { *(.data) }
    exports : {*(exports) }
	.bss : { *(COMMON) *(.bss) }
}