#pragma once
#include <stdint.h>
#include "console.h"

console_t* vga_init(uint16_t colour);
uint8_t vga_colour(uint8_t fg, uint8_t bg);

#define VGA_BUFFER_ADDR 0x000b8000