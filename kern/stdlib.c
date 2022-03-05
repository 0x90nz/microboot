#include <export.h>
#include "stdlib.h"
#include "io/vga.h"
#include "io/serial.h"
#include "io/keyboard.h"
#include "kernel.h"
#include "alloc.h"
#include "backtrace.h"

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
    while (n) {
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
 * @brief Tokenise a string reentrantly
 *
 * @param str pointer to the string, may be null on subsequent calls
 * @param delim a string containing all possible delimiters
 * @param saveptr used to save the internal state of the strtok_r function across
 *                multiple calls
 * @return char* the pointer to the next token
 */
// Based on the PDCLib version of strtok
char* strtok_r(char* str, const char* delim, char** saveptr)
{
    if (str != NULL) {
        // new string
        *saveptr = str;
    } else {
        if (*saveptr == NULL) {
            // stuck! no new string, nor any old string
            return NULL;
        }
        str = *saveptr;
    }

    const char* p = delim;
    while (*p && *str) {
        if (*str == *p) {
            // found delim
            str++;
            p = delim;
            continue;
        }

        p++;
    }

    if (!*str) {
        // nothing left
        *saveptr = str;
        return NULL;
    }

    // skip non-delim chrs
    *saveptr = str;

    while (**saveptr) {
        p = delim;
        while (*p) {
            if (**saveptr == *p++) {
                // found delim
                *((*saveptr)++) = '\0';
                return str;
            }
        }

        (*saveptr)++;
    }

    return str;
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
    static char* saveptr;
    return strtok_r(str, delim, &saveptr);
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
 * @brief Concatenate two strings together
 *
 * @param dst the destination
 * @param src the source
 */
void strcat(char* dst, const char* src)
{
    while (*dst)
        dst++;

    // Note: assignment _not_ equals, copies until *src == '\0'
    while ((*dst++ = *src++));
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
EXPORT_SYM(puts);
EXPORT_SYM(printf);

/**
 * @brief Print a single character
 *
 * @param c the character to print
 */
void putc(char c)
{
    if (!stdout)
        return;

    OOCALL(stdout, putc, c);
}
EXPORT_SYM(putc);

/**
 * @brief Get a single character. Blocks awaiting input
 *
 * @return char the character
 */
char getc()
{
    return OOCALL0(stdin, getc);
}
EXPORT_SYM(getc);

/**
 * @brief Print a single character to the debug output
 *
 * @param c the character
 * @param ignore a pointer to nothing, which is ignored. Used for compatability
 * with the printf library
 */
void debug_putc(char c, void* ignore)
{
    if (!dbgout)
        return;

    OOCALL(dbgout, putc, c);
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
        *str = getc();

        if (*str == '\b') {
            if (str > start) {
                if (should_echo)
                    putc(*str);

                *str-- = '\0'; // Blank over \b
                *str-- = '\0'; // blank over the character to erase
            } else if (str == start) {
                *str-- = '\0'; // in this case just blank over the \b
            }
            continue;
        }

        if (should_echo)
            putc(*str);
    } while (*str++ != '\n');
    *--str = '\0';
}
EXPORT_SYM(gets);

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
    while (*a && (*a == *b)) {
        a++; b++;
    }
    return *(const unsigned char*)a - *(const unsigned char*)b;
}

/**
 * @Brief Compare two strings without regard for the case of the characters. A
 * non-zero return value indicates that the strings differn in at least one
 * location
 *
 * @param a the first string
 * @param b the second string
 * @return int zero if identical (apart from case), non-zero otherwise
 */
int stricmp(const char* a, const char* b)
{
    while (*a && (tolower((unsigned char)*a) == tolower((unsigned char)*b))) {
        a++; b++;
    }
    return (unsigned char)*a - (unsigned char)*b;
}

/**
 * @brief Convert a character to lowercase. If the character is not a letter, or
 * is already lowercase, it will be returned as-is
 *
 * @param ch the character to make lowercase
 * @return int the lowercase character
 */
int tolower(int ch)
{
    if (ch >= 'A' && ch <= 'Z')
        return ch ^ 0x20;
    else
        return ch;
}

/**
 * @brief Compare two strings for a given number of characters
 *
 * @param a the first string
 * @param b the second string
 * @param n the number of characters to compare
 * @return int zero if identical for all indicated characters, non-zero otherwise
 */
int strncmp(const char* a, const char* b, size_t n)
{
    while (n && *a && (*a == *b)){
        ++a;
        ++b;
        --n;
    }

    if (n == 0) {
        return 0;
    } else {
        return (*(const unsigned char *)a - *(unsigned char *)b);
    }
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
 * @brief Find the start of the first occurence of a given substring within
 * a larger string
 *
 * @param haystack the string to search within
 * @param needle the substring to search for
 * @return const char* pointer to the start of the substring within
 * the larger string
 */
const char* strstr(const char* haystack, const char* needle)
{
    size_t nlen = strlen(needle);
    while (*haystack) {
        if (*haystack == *needle) {
            if (strncmp(haystack, needle, nlen) == 0)
                return haystack;
        }
        haystack++;
    }
    return NULL;
}

/**
 * @brief Return a pointer to the first instance of `c` within `s`. If no such
 * instance exists, returns NULL.
 *
 * @param s the string to search for the character in
 * @param c the character to search for
 * @return const char* pointer to the character if it exists; NULL otherwise.
 */
char* strchr(const char* s, int c)
{
    do {
        if (*s == c)
            return (char*)s;
    } while (*s++);
    return NULL;
}

/**
 * @brief Return a pointer to a new string, having the contents os the given
 * string. Memory for the string is obtained with `kalloc`.
 *
 * @param s the string to duplicate
 * @return pointer to the newly duplicated string
 */
char* strdup(const char* s)
{
    size_t size = strlen(s) + 1;
    char* str = kalloc(size);
    if (str)
        memcpy(str, s, size);
    return str;
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
#ifdef FAST_MEMOPS
    asm volatile(
        "rep stosb"
        :
        : "a" (value), "D" (memory), "c" (len)
        : "memory"
    );
#else
    uint8_t* ptr = memory;
    for (int i = 0; i < len; i++) {
        ptr[i] = value;
    }
#endif
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
#ifdef FAST_MEMOPS
    asm volatile(
        "rep movsl\n\t"         // move as much as we can long-sized
        "movl   %3, %%ecx\n\t"  // get the rest of the length
        "andl   $3, %%ecx\n\t"
        "jz     1f\n\t"         // perfectly long aligned? done if so
        "rep movsb\n\t"
        "1:"
        :
        : "S" (src), "D" (dst), "c" (len / 4), "r" (len)
        : "memory"
    );
#else
    const char* csrc = src;
    char* cdst = dst;
    while (len > 0) {
        *cdst++ = *csrc++;
        len--;
    }
#endif
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

void _assert(const char* expr_str, const char* file, int line, const char* func, int expr, const char* message)
{
    if (!expr) {
        char* assert_fmt = "ASSERT(%s) failed!\n%s() @ %s:%d\n: %s";
        printf(assert_fmt, expr_str, func, file, line, message);
        logf(LOG_FATAL, assert_fmt, expr_str, func, file, line, message);

        backtrace();
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

void dprintf_raw(const char* fmt, ...)
{
    va_list va;
    va_start(va, fmt);
    vfctprintf(debug_putc, NULL, fmt, va);
    va_end(va);
}

