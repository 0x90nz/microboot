#pragma once

#include <stdint.h>

struct gdt_entry {
    uint32_t base;
    uint32_t limit;
    uint32_t type;
    uint32_t flags;
};

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

enum gdt_flags {
    GDT_GRANULARITY     = (1 << 7),
    GDT_SIZE            = (1 << 6)
};

void gdt_init();