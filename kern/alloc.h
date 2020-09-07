#pragma once

#include <stddef.h>
#include <stdint.h>

void init_alloc(void* start, size_t size);
void* kalloc(size_t size);
void* kallocz(size_t size);
void kfree(void* ptr);
void kdumpmm();
void* krealloc(void* ptr, size_t new_size);
size_t alloc_total();
size_t alloc_used(int all);

#define MEM_BLOCK_MAGIC         0xbadbaddd
#define ALIGNMENT       64

enum mem_block_state {
    MEM_STATE_USED,
    MEM_STATE_FREE
};

typedef struct mem_block {
    uint32_t magic;
    size_t size;
    enum mem_block_state state;
    void* addr;
    struct mem_block* next;
} mem_block_t;