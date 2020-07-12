#include "vga.h"
#include "keyboard.h"
#include "interrupts.h"
#include "stdlib.h"

void hang() { while (1) { asm("hlt"); } }
void hlt() { asm("hlt"); }

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


char temp[32];
void print_hex(int num)
{
    itoa(num, temp, 16);
    puts(temp);
}

void print_int(int num)
{
    itoa(num, temp, 10);
    puts(temp);
}

int kernel_main()
{
    vga_init(vga_colour(VGA_WHITE, VGA_BLUE));

    interrupts_init();
    display_logo();

    uint16_t* size = (uint16_t*)0x07e00;
    print_int(*size);
    puts(" KiB\n");

    extern int main();
    main();

    hang();
}