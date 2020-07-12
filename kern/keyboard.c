#include <stdint.h>
#include "keyboard.h"
#include "kernel.h"

unsigned char scancode_pc104_lut[] = {
    0,
    0x13,       // ESC
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, // Special
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', 
    0, 0, 0,
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    0, 0, 0, ' '
};

uint8_t keyboard_poll_scancode()
{
    // Loop until we get something
    while (!(inb(KB_REG_STATUS) & KB_OUT_FULL)) { hlt(); }
    return inb(KB_REG_DATA);
}

unsigned char keyboard_convert_scancode(uint8_t scancode)
{
    if (scancode > sizeof(scancode_pc104_lut) / sizeof(unsigned char))
        return 0;

    return scancode_pc104_lut[scancode];
}

unsigned char keyboard_getchar()
{
    uint8_t code;
    do {
        code = keyboard_convert_scancode(keyboard_poll_scancode());
    } while(code == 0);
    return code;
}