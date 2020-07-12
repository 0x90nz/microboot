#pragma once

#include <stdint.h>

uint8_t keyboard_poll_scancode();
unsigned char keyboard_getchar(int retry);
int keyboard_available();

#define KB_REG_DATA         0x60
#define KB_REG_STATUS       0x64
#define KB_REG_COMMAND      0x64

#define KB_OUT_FULL         (1 << 0)
#define KB_IN_FULL          (1 << 1)

#define KB_LSHIFT           42
#define KB_RSHIFT           54

#define KB_UP_LSHIFT        170
#define KB_UP_RSHIFT        182