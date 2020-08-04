#include "io/pio.h"
#include "io/vga.h"
#include "io/pci.h"
#include "io/serial.h"
#include "io/keyboard.h"
#include "io/bios_drive.h"
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

void kernel_main(struct startup_info* start_info)
{
    vga_init(vga_colour(VGA_WHITE, VGA_BLUE));
    init_alloc((void*)0x01000000, start_info->extended2 * 64 * KiB);

    interrupts_init();
    keyboard_init();
    serial_init(SP_COM0_PORT);

    debugf("Loaded from bios disk %02x", start_info->drive_number);

    gdt_init();
    env_init();
    display_logo();
    env_put("prompt", "# ");

    // This is memory past 0x01000000 which is free to use
    printf("%d MiB free\n", (start_info->extended2 * 64) / 1024);

    uint8_t* buffer = kalloc(4096);
    kfree(buffer);
    bdrive_read(start_info->drive_number, 4, 0, buffer);

    extern int main();
    main();

    hang();
}