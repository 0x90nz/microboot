#include "alloc.h"
#include "stdlib.h"
#include "kernel.h"
#include <stdint.h>

void* mem_start = NULL;
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
    head->state = MEM_STATE_USED;

    last = head;
}

void* kalloc(size_t size)
{
    ASSERT(mem_start, "Allocator was used before initialised");

    for (mem_block_t* current = head; current; current = current->next)
    {
        if (current->state == MEM_STATE_FREE && current->size >= size)
        {
            current->state = MEM_STATE_USED;
            return (void*)current + sizeof(mem_block_t);
        }
    }

    mem_block_t* current = last + last->size + sizeof(mem_block_t);

    ASSERT((void*)current <= mem_start + mem_size, "Out of memory, cannot allocate");

    current->size = size;
    current->next = NULL;
    current->state = MEM_STATE_USED;
    last->next = current;
    last = current;

    return (void*)current + sizeof(mem_block_t);
}

void kfree(void* ptr)
{
    // We should only free addresses which we own
    ASSERT(ptr >= mem_start && ptr <= mem_start + mem_size, "Tried to free invalid address");

    mem_block_t* block = ptr - sizeof(mem_block_t);
    block->state = MEM_STATE_FREE;
}