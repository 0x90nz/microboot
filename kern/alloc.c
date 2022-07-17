/**
 * @file alloc.c
 * @brief Dynamic memory allocator
 */

#include "alloc.h"
#include "stdlib.h"
#include "kernel.h"
#include <export.h>
#include <stdint.h>

static void* mem_start = NULL;
static mem_block_t* head;
static size_t mem_size;

// #define ALLOC_DEBUG

/**
 * @brief Initialise the allocator
 * 
 * @param start the start of contiguous free memory
 * @param size the number of free bytes following the start
 */
void init_alloc(void* start, size_t size)
{
    mem_start = start;
    mem_size = size;
    head = start;
    head->next = NULL;
    head->size = 0;
    head->state = MEM_STATE_USED;
    head->magic = MEM_BLOCK_MAGIC;
    head->flags = MEM_CRITICAL;
}

/*
 * Align the requested size to the alignment boundary. This is useful to
 * increase reuse of blocks which are allocated
 */
static size_t aligned_size(size_t size)
{
    if (size % ALIGNMENT == 0) return size;
    return (size / ALIGNMENT) * ALIGNMENT + ALIGNMENT;
}

/**
 * @brief Dynamically allocate some memory. The returned pointer may be for a
 * space larger than that which was requested, but it will *always* be at least
 * as large as the space requested.
 *
 * @param size the size to allocate, in bytes
 * @return void* a pointer to the allocated memory
 */
void* kalloc(size_t size)
{
    ASSERT(mem_start, "Allocator was used before initialised");
    size_t allocated_size = aligned_size(size);

    if (allocated_size == 0)
        return NULL;

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
            current->flags = 0;
            return current->addr;
        }

        if (!current->next)
            break;
    }
#ifdef ALLOC_DEBUG
    debugf("Magic: %08x", current->magic);
#endif
    ASSERT(current->magic == MEM_BLOCK_MAGIC, "Memory corruption detected, magic value not present");

    mem_block_t* next = (void*)current + sizeof(mem_block_t) + current->size;
    ASSERT((void*)next <= mem_start + mem_size, "Out of memory, cannot allocate");

    next->size = allocated_size;
    next->next = NULL;
    next->state = MEM_STATE_USED;
    next->magic = MEM_BLOCK_MAGIC;
    next->flags = 0;
    next->addr = (void*)next + sizeof(mem_block_t);
    current->next = next;

#ifdef ALLOC_DEBUG
    debugf("Allocated memory at %08x", (void*)next + sizeof(mem_block_t));
#endif

    return next->addr;
}
EXPORT_SYM(kalloc);

/**
 * @brief Attempt to resize a dynamically allocated pointer to memory. If the
 * memory allocated is too small to contain the new size, then the requested
 * memory will be copied and the old memory freed.
 * 
 * @param ptr pointer to the memory to be reallocated
 * @param new_size the new size, as the entire size to be used not just the
 * additional size
 * @return void* a pointer to the new memory
 */
void* krealloc(void* ptr, size_t new_size)
{
    if (!ptr)
        return kalloc(new_size);

    ASSERT(ptr >= mem_start && ptr <= mem_start + mem_size, "Tried to realloc invalid address");
    mem_block_t* block = ptr - sizeof(mem_block_t);
    ASSERT(block->state == MEM_STATE_USED, "Tried to realloc unused block");
    ASSERT(block->magic == MEM_BLOCK_MAGIC, "Tried to realloc corrupted block");

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
EXPORT_SYM(krealloc);

/**
 * @brief Allocate memory and clear it
 * 
 * @param size the size to allocate, in bytes
 * @return void* a pointer to the allocated memory
 */
void* kallocz(size_t size)
{
    void* data = kalloc(size);
    memset(data, 0, size);
    return data;
}
EXPORT_SYM(kallocz);

/**
 * @brief Free allocated memory
 * 
 * @param ptr a pointer to the allocated memory
 */
void kfree(void* ptr)
{
#ifdef ALLOC_DEBUG
    debugf("Requested clear at %08x bytes", ptr);
#endif

    // We should only free addresses which we own
    ASSERT(ptr >= mem_start && ptr <= mem_start + mem_size, "Tried to free invalid address");

    mem_block_t* block = ptr - sizeof(mem_block_t);

    ASSERT(!(block->flags & MEM_CRITICAL), "Tried to free critical memory");
    ASSERT(block->state == MEM_STATE_USED, "Tried to free already free memory");
    ASSERT(block->magic == MEM_BLOCK_MAGIC, "Memory corruption detected, magic value not present");

    block->state = MEM_STATE_FREE;
}
EXPORT_SYM(kfree);

/**
 * @brief Check the validity of a memory address.
 *
 * The memory check can be performed either by means of a bounds check
 * (quick=1), or by means of a search of the entire chunk chain (quick=0).
 * Caution must be exercised with quick=1, because it could feasibly be fooled
 * into thinking a pointer is valid, when it is not. Conversely, with a system
 * having a large number of allocations, quick=0 can easily become quite slow.
 *
 * @param ptr the address to check for validity
 * @param quick whether to perform only a bounds check, or perform an
 * exhaustive search
 * @return int non-zero indicates valid, zero indicates invalid
 */
int alloc_valid_addr(void* ptr, int quick)
{
    if (ptr < mem_start || ptr > mem_start + mem_size) return 0;

    if (quick) {
        mem_block_t* block = ptr - sizeof(mem_block_t);
        return block->magic == MEM_BLOCK_MAGIC;
    }

    for (mem_block_t* current = head; current; current = current->next) {
        if (current->addr == ptr) {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Get the currently used amount of memory
 * 
 * @param all a non-zero value indicates that freed memory should also 
 * be counted
 * @return size_t the amount of memory currently in use, in bytes
 */
size_t alloc_used(int all)
{
    size_t total = 0;
    for (mem_block_t* current = head; current; current = current->next)
    {
        if (current->state != MEM_STATE_FREE || all)
            total += current->size;
    }
    return total;
}

/**
 * @brief Get the total amount of memory available
 * 
 * @return size_t the total amount of memory available
 */
size_t alloc_total()
{
    return mem_size;
}

/**
 * @brief Mark a region as critical. Attempts to free will fault
 * 
 * @param ptr the critical region
 */
void kmmcritical(void* ptr)
{
    ASSERT(ptr >= mem_start && ptr <= mem_start + mem_size, "Tried mark as critical an invalid address");
    mem_block_t* block = ptr - sizeof(mem_block_t);
    block->flags |= MEM_CRITICAL;
}

/**
 * @brief Dump memory management info, only for debugging
 */
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
