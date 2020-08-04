#pragma once

#include <stdint.h>

void hang();
void hlt();
void print_hex(int num);
void print_int(int num);

struct startup_info {
    uint16_t extended1;
    uint16_t extended2;
    uint16_t configured1;
    uint16_t configured2;
    uint16_t drive_number;
} __attribute__((packed));

#define KERNEL_CSEL     0x08
#define KERNEL_DSEL     0x10

#define KiB             1024
#define MiB             1048576

enum log_level {
    LOG_FATAL = 0,
    LOG_ERROR,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG,
    LOG_ALL
};

extern char* debug_names[];

#define DEBUG_LEVEL LOG_ALL