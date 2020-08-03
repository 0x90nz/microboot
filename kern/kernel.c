#include "io/pio.h"
#include "io/vga.h"
#include "io/pci.h"
#include "io/serial.h"
#include "io/keyboard.h"
#include "sys/interrupts.h"
#include "sys/gdt.h"
#include "sys/bios.h"
#include "stdlib.h"
#include "kernel.h"
#include "alloc.h"
#include "env.h"

char* debug_names[] = {
    "FATAL",
    "ERROR",
    "WARN",
    "INFO",
    "DEBUG",
    "ALL",
    "OFF"
};

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

void kernel_main(memory_info_t* meminfo)
{
    vga_init(vga_colour(VGA_WHITE, VGA_BLUE));
    init_alloc((void*)0x01000000, meminfo->extended2 * 64 * KiB);

    interrupts_init();
    keyboard_init();
    serial_init(SP_COM0_PORT);
    gdt_init();
    env_init();
    display_logo();
    env_put("prompt", "# ");

    // This is memory past 0x01000000 which is free to use
    printf("%d MiB free\n", (meminfo->extended2 * 64) / 1024);

    pci_test();

    extern int main();
    main();

    hang();
}