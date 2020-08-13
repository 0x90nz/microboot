#pragma once

#include <stdint.h>

void hang();
void hlt();
void print_hex(int num);
void print_int(int num);


struct kstart_info {
    uint8_t drive_number;
    uint32_t free_memory;
    void* memory_start;
};

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
void kernel_main(struct kstart_info* start_info);

#define DEBUG_LEVEL LOG_ALL