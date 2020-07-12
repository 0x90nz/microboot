#pragma once

#include <stdint.h>

void outb(uint16_t portnumber, uint8_t data);
uint8_t inb(uint16_t portnumber);

void hang();
void hlt();
void print_hex(int num);

typedef struct {
    uint16_t extended1;
    uint16_t extended2;
    uint16_t configured1;
    uint16_t configured2;
} memory_info_t;