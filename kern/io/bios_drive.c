#include "bios_drive.h"
#include "../sys/bios.h"
#include "../stdlib.h"
#include "../sys/addressing.h"

extern struct disk_addr low_mem_disk_addr;

void bdrive_read(uint8_t drive_num, uint16_t num_sectors, uint64_t lba, uint8_t* buffer) 
{
    ASSERT(num_sectors * 512 <= 4096, "Size would overflow low memory buffer");

    memset(&low_mem_disk_addr, 0, sizeof(struct disk_addr));
    low_mem_disk_addr.size = 16;
    low_mem_disk_addr.num_sectors = num_sectors;
    low_mem_disk_addr.low_lba = lba & 0xffffffff;
    low_mem_disk_addr.high_lba = lba >> 32;
    low_mem_disk_addr.buffer = (uint32_t)&low_mem_buffer;    

    struct int_regs regs;
    memset(&regs, 0, sizeof(struct int_regs));

    regs.eax = 0x4200;

    regs.esi = OFFOF((uint32_t)&low_mem_disk_addr);
    regs.ds = SEGOF((uint32_t)&low_mem_disk_addr);

    regs.edx = drive_num & 0xff;

    debugf("addr of struct %04x:%04x", SEGOF((uint32_t)&low_mem_disk_addr), OFFOF((uint32_t)&low_mem_disk_addr));

    bios_interrupt(0x13, &regs);

    ASSERT(!(regs.flags & EFL_CF), "Error reading from disk");

    memcpy(buffer, &low_mem_buffer, num_sectors * 512);
}