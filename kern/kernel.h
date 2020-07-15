#pragma once

#include <stdint.h>

void hang();
void hlt();
void print_hex(int num);
void print_int(int num);

typedef struct {
    uint16_t extended1;
    uint16_t extended2;
    uint16_t configured1;
    uint16_t configured2;
} memory_info_t;

#define KERNEL_CSEL     0x08
#define KERNEL_DSEL     0x10