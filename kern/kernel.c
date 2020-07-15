#include "vga.h"
#include "keyboard.h"
#include "interrupts.h"
#include "pio.h"
#include "stdlib.h"
#include "kernel.h"
#include "pci.h"
#include "alloc.h"

void hang() { while (1) { asm("hlt"); } }
void hlt() { asm("hlt"); }

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

int kernel_main(memory_info_t* meminfo)
{
    vga_init(vga_colour(VGA_WHITE, VGA_BLUE));
    init_alloc((void*)0x01000000, meminfo->extended2 * 64 * KiB);

    interrupts_init();
    keyboard_init();
    display_logo();

    // This is memory past 0x01000000 which is free to use
    print_int((meminfo->extended2 * 64) / 1024);
    puts(" MiB free\n");

    pci_test();

    extern int main();
    main();

    hang();
}