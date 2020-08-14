ENTRY(init)
OUTPUT_FORMAT("binary")

SECTIONS
{
    startinfo : { *(startinfo) }
    . = 0x7e00;
    .text : AT(4) { *(.text) }
	.rodata : { *(.rodata) }
	.data : { *(.data) }
	.bss : { *(COMMON) *(.bss) }
    LOADABLE_SIZE = . - 0x7e00;
}