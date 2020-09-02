#pragma once

#include <stdint.h>
#include <stddef.h>

#include "env.h"

void hang();
void hlt();

struct kstart_info {
    uint8_t drive_number;
    uint32_t free_memory;
    void* memory_start;
};

#define KERNEL_CSEL     0x08
#define KERNEL_DSEL     0x10

#define KiB             1024
#define MiB             1048576

#define STACK_MAGIC     0xc0ffebad

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
void dump_memory(void* input_buffer, size_t length);
env_t* get_rootenv();


#define DEBUG_LEVEL LOG_ALL