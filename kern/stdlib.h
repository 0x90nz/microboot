#pragma once

#include <stdint.h>
#include "printf.h"
#include "kernel.h"
#include "io/chardev.h"

// Standard library - ish functions
void itoa(int value, char* buffer, int base);
int atoi(const char* str);
void gets(char* str);
char getc();
void puts(const char* str);
void putc(char c);
int strcmp(const char* a, const char* b);
char* strcpy(char* dst, const char* src);
char* strtok(char* str, const char* delim);
size_t strlen(const char* str);
const char* strstr(const char* haystack, const char* needle);
char* strchr(const char* s, int c);
int strncmp(const char* a, const char* b, size_t n);
int stricmp(const char* a, const char* b);
void strcat(char* dst, const char* src);
int tolower(int ch);
char* strdup(const char* s);
void memset(void* memory, uint8_t value, size_t len);
void memcpy(void* dst, const void* src, size_t len);
int memcmp(const void* a, const void* b, size_t len);


// More specific OS functions
void cpuid(int i, uint32_t* regs);
void debug_putc(char c, void* ignore);
void dprintf_raw(const char* fmt, ...);
void set_log_level(enum log_level level);
enum log_level get_log_level();

// Handy macros

// For each `c` in `s`.
#define STR_FOREACH(c, s) for (int _strfe_i = 0, c = s[0]; s[_strfe_i]; c = s[++_strfe_i])

// Object-oriented(ish) call on a struct with function pointer members with arguments
#define OOCALL(o, m, ...) o->m(o, __VA_ARGS__)

// Object-oriented(ish) call on a struct with function pointer members with no arguments
#define OOCALL0(o, m) o->m(o)

#define MAX(a, b) \
    ({ __typeof__(a) _a = (a); \
       __typeof__(b) _b = (b); \
       _a > _b ? _a : _b; })

#define MIN(a, b) \
    ({ __typeof__(a) _a = (a); \
       __typeof__(b) _b = (b); \
       _a < _b ? _a : _b; })


// Assertions. Note that we use a ternary to force evaluation as an if, from which return an int
// to be used in the actual __assert function
void _assert(const char* expr_str, const char* file, int line, const char* func, int expr, const char* message);
#define ASSERT(expr, message)       _assert(#expr, __FILE__, __LINE__, __PRETTY_FUNCTION__, expr ? 1 : 0, message);

// Debug logging

void _debug_printf(enum log_level level, const char* file, int line, const char* func, const char* fmt, ...);

#define log(l, x)           _debug_printf(l, __FILE__, __LINE__, __PRETTY_FUNCTION__, x);
#define logf(l, x, ...)     _debug_printf(l, __FILE__, __LINE__, __PRETTY_FUNCTION__, x, __VA_ARGS__);
#define debug(x)            _debug_printf(LOG_DEBUG, __FILE__, __LINE__, __PRETTY_FUNCTION__, x);
#define debugf(x, ...)      _debug_printf(LOG_DEBUG, __FILE__, __LINE__, __PRETTY_FUNCTION__, x, __VA_ARGS__);

// use hand-optimised memory operations (e.g. memcpy, memset, etc.)
#define FAST_MEMOPS

