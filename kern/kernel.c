#include "vga.h"
#include "keyboard.h"
#include "interrupts.h"

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

void swap(char* x, char* y)
{
    char temp = *x;
    *x = *y;
    *y = temp;
}

void reverse(char* buffer, int i, int j)
{
    while (i < j)
        swap(&buffer[i++], &buffer[j--]);
}

void itoa(int value, char* buffer, int base)
{
    if (base < 2 || base > 32)
        return;

    int n = value < 0 ? -value : value;

    int i = 0;
    while (n)
    {
        int rem = n % base;
        if (rem >= 10)
            buffer[i++] = 'A' + (rem - 10);
        else
            buffer[i++] = '0' + rem;
        n /= base;
    }

    if (i == 0)
        buffer[i++] = '0';

    if (value < 0 && base == 10)
        buffer[i++] = '-';
    
    buffer[i] = '\0';

    reverse(buffer, 0, i - 1);
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

void print_hex(int num)
{
    char temp[32];
    itoa(num, temp, 16);
    vga_puts(temp);
}

int kernel_main()
{
    vga_init(vga_colour(VGA_GREEN, VGA_BLACK));

    interrupts_init();

    display_logo();
    vga_puts(" >>> ");

    hang();
}