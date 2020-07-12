#include "vga.h"

void hang() { while (1) { asm("hlt"); } }

// Byte sized port access
void outb(uint16_t portnumber, uint8_t data)
{
	asm volatile("outb %%al, %%dx" :: "d" (portnumber), "a" (data));
}
 
uint8_t inb(uint16_t portnumber)
{
	uint8_t result;
	asm volatile("inb %%dx, %%al" : "=a" (result) : "d" (portnumber));
	return result;
}

void display_logo()
{
    vga_puts("\n");
    int offset = 16;
    vga_pad(offset); vga_puts("           ##                            ##\n");
    vga_pad(offset); vga_puts("           ##                            ##\n");
    vga_pad(offset); vga_puts(" ##    ##  ########   #######   ####### #####\n");
    vga_pad(offset); vga_puts(" ##    ##  ##    ### ###   ### ###   ### ##\n");
    vga_pad(offset); vga_puts(" ##    ##  ##     ## ##     ## ##     ## ##\n");
    vga_pad(offset); vga_puts(" ##    ##  ###   ### ###   ### ###   ### ##\n");
    vga_pad(offset); vga_puts(" # ####  # ########   #######   #######  #####\n");
    vga_pad(offset); vga_puts(" ##\n");
    vga_pad(offset); vga_puts(" ##\n");
}

int kernel_main()
{
    vga_init();
    display_logo();
    hang();
}