ENTRY(main)
OUTPUT(elf)

SECTIONS
{
    .text 0xf000 : { *(.text) }
    .rodata :  { *(.rodata) }
	.data : { *(.data) }
	.bss : { *(COMMON) *(.bss) }
}