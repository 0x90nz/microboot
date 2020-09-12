ENTRY(main)
OUTPUT(elf)

SECTIONS
{
    .text : { *(.text) }
    .rodata :  { *(.rodata) }
	.data : { *(.data) }
	.bss : { *(COMMON) *(.bss) }
}