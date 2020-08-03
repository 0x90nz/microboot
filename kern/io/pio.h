#pragma once

#include <stdint.h>

void outb(uint16_t portnumber, uint8_t data);
uint8_t inb(uint16_t portnumber);
uint16_t inw(uint16_t portnumber);
void outw(uint16_t portnumber, uint16_t data);
void outl(uint16_t portnumber, uint32_t data);
uint32_t inl(uint16_t portnumber);