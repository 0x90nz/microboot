#pragma once

#include <stdint.h>

void outb(uint16_t portnumber, uint8_t data);
uint8_t inb(uint16_t portnumber);

void hang();
void hlt();