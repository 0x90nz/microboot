#include "stdlib.h"
#include "io/vga.h"
#include "io/serial.h"
#include "io/keyboard.h"
#include "kernel.h"

static int should_echo = 1;
static enum log_level log_level = DEBUG_LEVEL;

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

int atoi(const char* str)
{
    int n = 0;
    int neg_flag = 0;

    switch (*str) {
        case '-': neg_flag = 1;
        case '+': str++;
    }

    while (*str != '\0')
        n = 10 * n - (*str++ - '0');
    
    return neg_flag ? n : -n;
}

char* strtok(char* str, const char* delim)
{
    static char* buf;
    static int last = 0;
    if(str != NULL) { buf = str; last = 0; }
    if(buf == NULL || buf[0] == '\0' || last) return NULL;

    char *ret = buf, *b;
    const char *d;
 
    for(b = buf; *b !='\0'; b++) {
        for(d = delim; *d != '\0'; d++) {
            if(*b == *d) {
                *b = '\0';
                buf = b+1;
                if(b == ret) { 
                    ret++; 
                    continue; 
                }
                return ret;
            }
        }
    }

    last = 1;
    return buf;
}

size_t strlen(const char* str)
{
    size_t len = 0;
    while (*str++) len++;
    return len;
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

char* strcpy(char* dst, const char* src)
{
    char* tmp = dst;
    while((*dst++ = *src++) != '\0');
    return tmp;
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

void set_log_level(enum log_level level)
{
    log_level = level;
}

enum log_level get_log_level()
{
    return log_level;
}

void _assert(const char* file, int line, const char* func, int expr, const char* message)
{
    if (!expr)
    {
        char* assert_fmt = "%s() @ %s:%d [%s]";
        puts("Assertion failed!\n");
        printf(assert_fmt, func, file, line, message);
        logf(LOG_FATAL, assert_fmt, func, file, line, message);

        hang();
    }
}

void _debug_printf(enum log_level level, const char* file, int line, const char* func, const char* fmt, ...)
{
    if (level > log_level)
        return;

    va_list va;
    va_start(va, fmt);
    fctprintf(debug_putc, NULL, "[%s] %s: ", debug_names[level], func);
    vfctprintf(debug_putc, NULL, fmt, va);
    debug_putc('\n', NULL);
    va_end(va);
}
