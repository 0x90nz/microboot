#include "vga.h"
#include "keyboard.h"
#include "interrupts.h"
#include "pio.h"
#include "stdlib.h"
#include "kernel.h"
#include "pci.h"
#include "alloc.h"
#include "serial.h"
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
    env_init();
    display_logo();

    // This is memory past 0x01000000 which is free to use
    printf("%d MiB free\n", (meminfo->extended2 * 64) / 1024);

    env_put("test", "123");
    debugf("value was: %s", env_get("test"));

    pci_test();

    extern int main();
    main();

    hang();
}