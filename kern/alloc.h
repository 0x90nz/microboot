#pragma once

#include <stddef.h>

void* alloc(size_t size);
void free(void* ptr);

typedef struct mem_block {
    size_t size;
    struct mem_block* next;
} mem_block_t;