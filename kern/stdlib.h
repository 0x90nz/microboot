#pragma once

#include <stddef.h>
#include <stdint.h>
#include "printf.h"

void itoa(int value, char* buffer, int base);
void gets(char* str);
char getc();
void puts(const char* str);
void putc(char c);
int strcmp(const char* a, const char* b);
void memset(void* memory, uint8_t value, size_t len);
void memcpy(void* dst, const void* src, size_t len);
void debug_putc(char c, void* ignore);


// Assertions. Note that we use a ternary to force evaluation as an if, from which return an int
// to be used in the actual __assert function
void _assert(const char* file, int line, const char* func, int expr, const char* message);
#define ASSERT(expr, message)       _assert(__FILE__, __LINE__, __PRETTY_FUNCTION__, expr ? 1 : 0, message);


void _debug_printf(const char* file, int line, const char* func, const char* fmt, ...);
#define dprintf(x, ...)     _debug_printf(__FILE__, __LINE__, __PRETTY_FUNCTION__, x, __VA_ARGS__);
