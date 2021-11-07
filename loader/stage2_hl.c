#include "../kern/kernel.h"
#include "stage2.h"

// Linker symbols
extern int _kload_addr, _kphys_addr;
extern int _kend, _kbss_end;

BSCODE static void memcpy(void* dst, const void* src, uint32_t len)
{
    asm volatile(
        "rep movsl\n\t"         // move as much as we can long-sized
        "movl   %3, %%ecx\n\t"  // get the rest of the length
        "andl   $3, %%ecx\n\t"
        "jz     1f\n\t"         // perfectly long aligned? done if so
        "rep movsb\n\t"
        "1:"
        :
        : "S" (src), "D" (dst), "c" (len / 4), "r" (len)
        : "memory"
    );
}

BSCODE static void memset(void* memory, uint8_t value, uint32_t len)
{
    asm volatile(
        "rep stosb"
        :
        : "a" (value), "D" (memory), "c" (len)
        : "memory"
    );
}

struct kstart_info info;

BSCODE void stage2_hl(struct startup_info* start_info)
{
    // The total size (in bytes) of our kernel
    uint32_t ksize = (uint32_t)&_kend - (uint32_t)&_kload_addr;

    memcpy((uint8_t*)&_kload_addr, (uint8_t*)&_kphys_addr, ksize);

    // Zero out the BSS
    uint8_t* bss = (uint8_t*)&_kend;
    uint8_t* bss_end = (uint8_t*)&_kbss_end;
    memset(bss, 0, bss_end - bss);

    info.drive_number = start_info->drive_number;
    info.free_memory = start_info->extended2;
    info.memory_start = bss_end;

    kernel_main(&info);

    asm("cli");
    while (1) { asm("hlt"); }
}
