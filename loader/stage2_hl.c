#include "../kern/kernel.h"

struct startup_info {
    uint16_t extended1;
    uint16_t extended2;
    uint16_t configured1;
    uint16_t configured2;
    uint16_t drive_number;
} __attribute__((packed));


#define BSCODE      __attribute__ ((section("bootstrap")))

// Linker symbols
// extern int _low_start, _low_end, _kstart, _kend, _kbss_end;
// extern int HIGHMEM_SIZE, HIGHMEM_SIZE_BSS, KERNEL_SIZE;

extern int _kload_addr, _kphys_addr;
extern int _kend, _kbss_end;

// A reasonably optimised copy. In future I might rewrite this in
// pure assembly, but this should be good enough for now.
// BSCODE void fastcpy(void* dst, void* src, uint32_t sz)
// {
//     // Number of 4 byte sections to copy
//     uint32_t lcopy = sz / 4;
//     // Number of bytes left over
//     uint8_t bcopy = sz % 4;

//     uint32_t* ldst = (uint32_t*)dst;
//     uint32_t* lsrc = (uint32_t*)src;

//     for (uint32_t i = 0; i < lcopy; i++)
//     {
//         ldst[i] = lsrc[i];
//     }

//     uint8_t* bsrc = (uint8_t*)(lsrc + lcopy);
//     uint8_t* bdst = (uint8_t*)(ldst + lcopy);

//     for (uint8_t i = 0; i < bcopy; i++)
//     {
//         bdst[i] = bsrc[i];
//     }
// }

BSCODE void slowcpy(void* dst, void* src, uint32_t sz)
{
    uint8_t* bsrc = (uint8_t*)src;
    uint8_t* bdst = (uint8_t*)dst;
    for (uint32_t i = 0; i < sz; i++) {
        bdst[i] = bsrc[i];
    }
}

struct kstart_info info;

BSCODE void stage2_hl(struct startup_info* start_info)
{
    // The total size (in bytes) of our kernel
    uint32_t ksize = (uint32_t)&_kend - (uint32_t)&_kload_addr;

    slowcpy((uint8_t*)&_kload_addr, (uint8_t*)&_kphys_addr, ksize);
    // fastcpy((uint8_t*)&_kload_addr, (uint8_t*)&_kphys_addr, ksize);

    // Zero out the BSS
    uint8_t* bss = (uint8_t*)&_kend;
    uint8_t* bss_end = (uint8_t*)&_kbss_end;
    while (bss < bss_end)
        *bss++ = 0;

    info.drive_number = start_info->drive_number;
    info.free_memory = start_info->extended2;
    info.memory_start = bss_end;

    kernel_main(&info);

    asm("cli");
    while (1) { asm("hlt"); }
}