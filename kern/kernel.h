#pragma once

#include <stdint.h>
#include <stddef.h>

#include "env.h"
#include "io/chardev.h"
#include "io/console.h"

void hang();
void hlt();
void yield();

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
void kpoweroff();
uint32_t kticks();
env_t* get_rootenv();

// eventually replace with a more unified device manager or file IO?
extern console_t* console;
extern chardev_t* stdout;
extern chardev_t* stdin;
extern chardev_t* dbgout;

#define DEBUG_LEVEL LOG_ALL
