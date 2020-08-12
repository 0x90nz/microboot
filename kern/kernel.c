#include "io/pio.h"
#include "io/vga.h"
#include "io/pci.h"
#include "io/serial.h"
#include "io/keyboard.h"
#include "fs/fs.h"
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

    gdt_init();
    env_init();
    display_logo();
    env_put("prompt", "# ");
    env_put("root", &start_info->drive_number);

    debugf("Loaded from bios disk %02x", start_info->drive_number);
    
    extern int _kstart, _kend;
    uint32_t kstart = (uint32_t)&_kstart;
    uint32_t kend = (uint32_t)&_kend;
    debugf("kernel bounds: start=%08x, end=%08x, size=%08x", kstart, kend, kend - kstart);

    // This is memory past 0x01000000 which is free to use
    printf("%d MiB free\n", (start_info->extended2 * 64) / 1024);

    fs_init(start_info->drive_number);

    extern int main();
    main();

    hang();
}