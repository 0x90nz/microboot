OUTPUT_FORMAT(binary)
STARTUP(build/stage2.o)

SECTIONS
{
	/* Keep the low memory sections in low memory */
	. = 0x7e00;
	_low_start = .;
	bootstrap : { *(bootstrap) }
	bios : { *(bios) }
	_low_end = .;

	_bs_size = _low_end - _low_start;

	_kload_addr = 0x01000000;
	. = _kload_addr;

	.text ALIGN(4K) :  AT(ADDR(.text) - _kload_addr + _low_end) { *(.text) }

	exports : { 
			_kexp_start = .;
			*(exports)
			_kexp_end = .;
	}
	

	.rodata ALIGN(4K) :  { *(.rodata) }
	.data ALIGN(4K) : { *(.data) }
	
	_kend = .;
	.bss ALIGN(4K) : { *(COMMON) *(.bss) }
	_kbss_end = .;

	_kphys_addr = LOADADDR(.text);
	KERNEL_SIZE = _bs_size + (_kend - _kload_addr);
}
