#include <stdint.h>
#include "keyboard.h"
#include "kernel.h"
#include "interrupts.h"
#include "pio.h"

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

unsigned char scancode_pc104_shift_lut[] = {
    0,
    0x13,       // ESC
    '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, // Special
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', 
    0, 0, 0,
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
    0, 0, 0, ' '
};

static int shift = 0;
static int available = 0;

void keyboard_handle_irq(uint32_t int_no, uint32_t err_no)
{
    available = 1;
}

void keyboard_init()
{
    register_handler(IRQ_TO_INTR(1), keyboard_handle_irq);
}

uint8_t keyboard_poll_scancode()
{
    // Loop until we get something
    while (!available) { hlt(); }
    available = 0;
    return inb(KB_REG_DATA);
}

int keyboard_available()
{
    return available;
}

unsigned char keyboard_convert_scancode(uint8_t scancode)
{
    if (scancode > sizeof(scancode_pc104_lut) / sizeof(unsigned char))
        return 0;

    return shift ? scancode_pc104_shift_lut[scancode] : scancode_pc104_lut[scancode];
}

unsigned char keyboard_getchar(int retry)
{
    uint8_t code = 0;
    do {
        uint8_t raw_code = keyboard_poll_scancode();

        if (raw_code == KB_LSHIFT || raw_code == KB_RSHIFT)
            shift = 1;
        else if (raw_code == KB_UP_LSHIFT || raw_code == KB_UP_LSHIFT)
            shift = 0;
        else
            code = keyboard_convert_scancode(raw_code);
    } while(retry && code == 0);
    return code;
}