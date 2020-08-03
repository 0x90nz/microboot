#pragma once

#include <stddef.h>
#include <stdint.h>
#include "printf.h"
#include "kernel.h"

// Standard library - ish functions
void itoa(int value, char* buffer, int base);
void gets(char* str);
char getc();
void puts(const char* str);
void putc(char c);
int strcmp(const char* a, const char* b);
void memset(void* memory, uint8_t value, size_t len);
void memcpy(void* dst, const void* src, size_t len);

void debug_putc(char c, void* ignore);
void set_log_level(enum log_level level);

// Assertions. Note that we use a ternary to force evaluation as an if, from which return an int
// to be used in the actual __assert function
void _assert(const char* file, int line, const char* func, int expr, const char* message);
#define ASSERT(expr, message)       _assert(__FILE__, __LINE__, __PRETTY_FUNCTION__, expr ? 1 : 0, message);

// Debug logging

void _debug_printf(enum log_level level, const char* file, int line, const char* func, const char* fmt, ...);

#define log(l, x)           _debug_printf(l, __FILE__, __LINE__, __PRETTY_FUNCTION__, x);
#define logf(l, x, ...)     _debug_printf(l, __FILE__, __LINE__, __PRETTY_FUNCTION__, x, __VA_ARGS__);
#define debug(x)            _debug_printf(LOG_DEBUG, __FILE__, __LINE__, __PRETTY_FUNCTION__, x);
#define debugf(x, ...)      _debug_printf(LOG_DEBUG, __FILE__, __LINE__, __PRETTY_FUNCTION__, x, __VA_ARGS__);
