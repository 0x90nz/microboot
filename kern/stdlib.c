#include "stdlib.h"
#include "vga.h"
#include "keyboard.h"
#include "kernel.h"
#include "serial.h"
#include <stddef.h>

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

void debug_putc(char c, void* ignore)
{
    serial_putc(c);
}

void set_echo(int echo)
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

void memset(void* memory, uint8_t value, size_t len)
{
    uint8_t* ptr = memory;
    for (int i = 0; i < len; i++)
    {
        ptr[i] = value;
    }
}

void memcpy(void* dst, const void* src, size_t len)
{
    uint8_t* lsrc = (uint8_t*)src;
    uint8_t* ldst = (uint8_t*)dst;

    for (size_t i = 0; i < len; i++)
    {
        ldst[i] = lsrc[i];
    }
}

void _assert(const char* file, int line, const char* func, int expr, const char* message)
{
    if (!expr)
    {
        puts("Assertion failed!\n");
        printf("%s() @ %s %d [%s]", func, file, line, message);

        hang();
    }
}

void _debug_printf(enum log_level level, const char* file, int line, const char* func, const char* fmt, ...)
{
    if (level < DEBUG_LEVEL)
        return;

    va_list va;
    va_start(va, fmt);
    fctprintf(debug_putc, NULL, "[%s %s] ", debug_names[level], func);
    vfctprintf(debug_putc, NULL, fmt, va);
    debug_putc('\n', NULL);
    va_end(va);
}
