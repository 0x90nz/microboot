#include "bios_drive.h"
#include "../sys/bios.h"
#include "../stdlib.h"

void bdrive_init()
{

}

static uint16_t cyl_sect_to_cx(uint16_t cylinders, uint16_t sectors)
{
    return (cylinders & 0x3ff) << 6 | (sectors & 0x3f);
}


void bdrive_read_sectors(
    uint8_t drive_number, 
    uint32_t cyl, uint32_t head, uint32_t sect, 
    uint8_t* buffer, uint8_t num_sectors)
{
    struct int_regs regs;
    memset(&regs, 0, sizeof(struct int_regs));
    regs.eax = 0x0200 | (num_sectors & 0xff);
    regs.ecx = cyl_sect_to_cx(cyl, sect);
    regs.edx = (head & 0xff) << 8 | drive_number;

    regs.es = 0;
    regs.ebx = (uint32_t)&low_mem_buffer;

    bios_interrupt(0x13, &regs);

    ASSERT(!(regs.flags & EFL_CF), "Error reading from drive");

    memcpy(buffer, &low_mem_buffer, 512 * num_sectors);
}