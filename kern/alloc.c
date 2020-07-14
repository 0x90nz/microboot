#include "alloc.h"
#include <stdint.h>

void* mem_start;
mem_block_t* head;
mem_block_t* last;
size_t mem_size;

void init_alloc(void* start, size_t size)
{
    mem_start = start;
    mem_size = size;
    head = start;
    head->next = NULL;
    head->size = 0;

    last = head;
}

void* alloc(size_t size)
{
    last = head + 1;
}

void free(void* ptr)
{
    
}