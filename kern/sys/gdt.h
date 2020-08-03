#pragma once

#include <stdint.h>

struct gdt_entry {
    uint32_t base;
    uint32_t limit;
    uint32_t type;
};

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

void gdt_init();