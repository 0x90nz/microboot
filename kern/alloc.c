#include "alloc.h"
#include "stdlib.h"
#include "kernel.h"
#include <stdint.h>

void* mem_start = NULL;
mem_block_t* head;
size_t mem_size;

#define ALIGNMENT       64

// #define ALLOC_DEBUG

void init_alloc(void* start, size_t size)
{
    mem_start = start;
    mem_size = size;
    head = start;
    head->next = NULL;
    head->size = 0;
    head->state = MEM_STATE_USED;
    head->magic = MEM_BLOCK_MAGIC;
}

/**
 * Align the requested size to the alignment boundary
 * Means that we can re-use more blocks easier, and because
 * we're naive and don't combine blocks, this is good.
 */ 
size_t aligned_size(size_t size)
{
    if (size % ALIGNMENT == 0) return size;
    return (size / ALIGNMENT) * ALIGNMENT + ALIGNMENT;
}

void* kalloc(size_t size)
{
    ASSERT(mem_start, "Allocator was used before initialised");
    size_t allocated_size = aligned_size(size);

#ifdef ALLOC_DEBUG
    debugf("Requested allocation of %d bytes (aligned to %d)", size, allocated_size);
#endif

    mem_block_t* current;
    for (current = head; ; current = current->next)
    {
        if (current->state == MEM_STATE_FREE && current->size >= allocated_size)
        {
            current->state = MEM_STATE_USED;

#ifdef ALLOC_DEBUG
            debugf("Reused memory at %08x", (void*)current + sizeof(mem_block_t));
#endif
            current->addr = (void*)current + sizeof(mem_block_t);
            return current->addr;
        }

        if (!current->next)
            break;
    }

    ASSERT(current->magic == MEM_BLOCK_MAGIC, "Memory corruption detected, magic value not present");

    mem_block_t* next = (void*)current + sizeof(mem_block_t) + current->size;
    ASSERT((void*)next <= mem_start + mem_size, "Out of memory, cannot allocate");

    next->size = allocated_size;
    next->next = NULL;
    next->state = MEM_STATE_USED;
    next->magic = MEM_BLOCK_MAGIC;
    next->addr = (void*)next + sizeof(mem_block_t);
    current->next = next;

#ifdef ALLOC_DEBUG
    debugf("Allocated memory at %08x", (void*)next + sizeof(mem_block_t));
#endif

    return next->addr;
}

/**
 * Attempt to resize memory. If the memory is too small to contain the new size
 * requested then the memory will be copied and the old pointer freed.
 */ 
void* krealloc(void* ptr, size_t new_size)
{
    ASSERT(ptr >= mem_start && ptr <= mem_start + mem_size, "Tried to realloc invalid address");
    mem_block_t* block = ptr - sizeof(mem_block_t);
    
    if (new_size < block->size) {
        block->size = new_size;
        return ptr;
    } else {
        void* new_block = kalloc(new_size);
        memcpy(new_block, ptr, block->size);
        kfree(ptr);
        return new_block;
    }
}

/**
 * Allocate memory and clear it before returning the pointer
 */ 
void* kallocz(size_t size)
{
    void* data = kalloc(size);
    memset(data, 0, size);
    return data;
}

void kfree(void* ptr)
{
    // We should only free addresses which we own
    ASSERT(ptr >= mem_start && ptr <= mem_start + mem_size, "Tried to free invalid address");

#ifdef ALLOC_DEBUG
    debugf("Requested clear at %08x bytes", ptr);
#endif

    mem_block_t* block = ptr - sizeof(mem_block_t);
    
    ASSERT(block->state == MEM_STATE_USED, "Tried to free already free memory");
    ASSERT(block->magic == MEM_BLOCK_MAGIC, "Memory corruption detected, magic value not present");

    block->state = MEM_STATE_FREE;
}

size_t alloc_used()
{
    size_t total = 0;
    for (mem_block_t* current = head; current; current = current->next)
    {
        total += current->size;
    }
    return total;
}

void kdumpmm()
{
    debugf("sizeof(mem_block_t) = %d", sizeof(mem_block_t));
    int blk = 0;
    for (mem_block_t* current = head; current; current = current->next, blk++)
    {
        debugf("block %04d, size: %d bytes, at: %08x", 
            blk, 
            current->size, 
            (void*)current + sizeof(mem_block_t)
        );
    }
}