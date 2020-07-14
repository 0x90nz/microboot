#pragma once

void itoa(int value, char* buffer, int base);
void gets(char* str);
char getc();
void puts(const char* str);
void putc(char c);
int strcmp(const char* a, const char* b);

void __assert(const char* file, int line, const char* func, int expr, const char* message);
#define ASSERT(expr, message)       __assert(__FILE__, __LINE__, __PRETTY_FUNCTION__, expr, message);