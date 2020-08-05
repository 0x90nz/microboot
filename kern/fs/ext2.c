#include "ext2.h"
#include "../stdlib.h"
#include "../alloc.h"
#include "../io/bios_drive.h"

static struct ext2_superblock* superblock;

void ext2_init(uint8_t drive_num, uint32_t start_lba, uint32_t num_sectors)
{
    debugf("initialising on drive %02x at start lba %08x", drive_num, start_lba);
    superblock = kalloc(1024);
    bdrive_read(drive_num, 2, start_lba + 2, superblock);

    debugf("superblock magic %04x", superblock->magic);
    debugf("first data block: %08x", superblock->first_data_block);
}