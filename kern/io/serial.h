#pragma once

#define SP_COM0_PORT        0x3f8
#define SP_COM1_PORT        0x2f8
#define SP_COM2_PORT        0x3e8
#define SP_COM4_PORT        0x2e8

void serial_init(uint16_t port);
void serial_putc(char c);
char serial_getc();