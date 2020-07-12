#include "stdlib.h"
#include "vga.h"
#include "keyboard.h"

static int should_echo = 1;

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

void puts(const char* str)
{
    vga_puts(str);
}

void putc(char c)
{
    vga_putc(c);
}

char getc()
{
    return keyboard_getchar();
}

char set_echo(int echo)
{
    should_echo = echo;
}

void gets(char* str)
{
    do {
        *str = keyboard_getchar();
        
        if (should_echo)
            putc(*str);            
    } while (*str++ != '\n');
    *--str = '\0';
}
