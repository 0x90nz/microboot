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

/**
 * @brief Convert an integer to an ascii value
 * 
 * @param value the integer value
 * @param buffer the buffer to place the result in
 * @param base the base to output the number in
 */
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

/**
 * @brief Convert an ASCII string which is a base 10 number into its integer
 * representation. If the string contains non-digit characters, the behaviour 
 * is undefined.
 * 
 * @param str the string to convert
 * @return int the integer result
 */
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

/**
 * @brief Tokenise a string
 * 
 * @param str pointer to the string, may be null on subsequent calls
 * @param delim a string containing all possible delimiters
 * @return char* the pointer to the next token
 */
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

/**
 * @brief Determine the length of a given string
 * 
 * @param str the string
 * @return size_t the length of the string
 */
size_t strlen(const char* str)
{
    size_t len = 0;
    while (*str++) len++;
    return len;
}

/**
 * @brief Print a string
 * 
 * @param str the string to print
 */
void puts(const char* str)
{
    while (*str)
        putc(*str++);
}

/**
 * @brief Print a single character
 * 
 * @param c the character to print
 */
void putc(char c)
{
    if (!stdout)
        return;

    console_putc(stdout, c);
}

/**
 * @brief Get a single character. Blocks awaiting input
 * 
 * @return char the character
 */
char getc()
{
    return keyboard_getchar(1);
}

/**
 * @brief Print a single character to the debug output
 * 
 * @param c the character
 * @param ignore a pointer to nothing, which is ignored. Used for compatability
 * with the printf library
 */
void debug_putc(char c, void* ignore)
{
    serial_putc(c);
}

/**
 * @brief Set whether input is echoed back to the user. A non-zero indicates
 * input should be echoed while a zero value indicates it should not
 * 
 * @param echo the value to set echo as
 */
void set_echo(int echo)
{
    should_echo = echo;
}

/**
 * @brief Read a string from input into the provided buffer
 * 
 * @param str the buffer
 */
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

/**
 * @brief Compare two strings. A non-zero return value indicates that the strings
 * differ in at least one location
 * 
 * @param a the first string
 * @param b the second string
 * @return int zero if identical, non-zero otherwise
 */
int strcmp(const char* a, const char* b)
{
    while (*a && (*a == *b))
    {
        a++; b++;
    }
    return *(const unsigned char*)a - *(const unsigned char*)b;
}

/**
 * @brief Copy a string to the location pointed to by dst
 * 
 * @param dst where to copy the string to
 * @param src where to copy the string from
 * @return char* where the string was copied to
 */
char* strcpy(char* dst, const char* src)
{
    char* tmp = dst;
    while((*dst++ = *src++) != '\0');
    return tmp;
}

/**
 * @brief Set a region of memory to a specific value
 * 
 * @param memory a pointer to the start of the memory to set
 * @param value the value to set the region of memory to
 * @param len the amount of memory (in bytes) to set
 */
void memset(void* memory, uint8_t value, size_t len)
{
    uint8_t* ptr = memory;
    for (int i = 0; i < len; i++)
    {
        ptr[i] = value;
    }
}

/**
 * @brief Copy one region of memory to another
 * 
 * @param dst the destination to copy memory to
 * @param src the area to copy memory from
 * @param len the amount (in bytes) of memory to set
 */
void memcpy(void* dst, const void* src, size_t len)
{
    uint8_t* lsrc = (uint8_t*)src;
    uint8_t* ldst = (uint8_t*)dst;

    for (size_t i = 0; i < len; i++)
    {
        ldst[i] = lsrc[i];
    }
}

int memcmp(const void* a, const void* b, size_t len)
{
    const uint8_t* la = a;
    const uint8_t* lb = b;

    for (size_t i = 0; i < len; i++) {
        if (la[i] != lb[i])
            return la[i] - lb[i];
    }

    return 0;
}

/**
 * @brief Set the debug log level
 * 
 * @param level the level to set
 */
void set_log_level(enum log_level level)
{
    log_level = level;
}

/**
 * @brief Get the current debug log level
 * 
 * @return enum log_level the current log level
 */
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
