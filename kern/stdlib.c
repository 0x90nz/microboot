#include "stdlib.h"
#include "vga.h"
#include "keyboard.h"
#include "kernel.h"

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
    return keyboard_getchar(1);
}

char set_echo(int echo)
{
    should_echo = echo;
}

void gets(char* str)
{
    const char* start = str;
    do {
        *str = keyboard_getchar(1);

        if (*str == '\b')
        {
            if (str > start)
            {
                if (should_echo)
                    putc(*str);

                *str-- = '\0'; // Blank over \b
                *str-- = '\0'; // blank over the character to erase
            }
            else if (str == start)
            {
                *str-- = '\0'; // in this case just blank over the \b
            }
            continue;
        }

        if (should_echo)
            putc(*str);
    } while (*str++ != '\n');
    *--str = '\0';
}

int strcmp(const char* a, const char* b)
{
    while (*a && (*a == *b))
    {
        a++; b++;
    }
    return *(const unsigned char*)a - *(const unsigned char*)b;
}

void __assert(const char* file, int line, const char* func, int expr, const char* message)
{
    if (!expr)
    {
        puts("Assertion failed!\n");
        puts(func);
        puts("() @ ");
        puts(file);
        puts(":");
        print_int(line);
        puts(" (");
        puts(message);
        puts(")");

        while (1) { hlt(); }
    }
}
