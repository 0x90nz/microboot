#pragma once

#include <stddef.h>

void init_alloc(void* start, size_t size);
void* kalloc(size_t size);
void kfree(void* ptr);

enum mem_block_state {
    MEM_STATE_USED,
    MEM_STATE_FREE
};

typedef struct mem_block {
    size_t size;
    int state;
    struct mem_block* next;
} mem_block_t;