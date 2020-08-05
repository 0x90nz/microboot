#include "ext2.h"
#include "../stdlib.h"
#include "../alloc.h"
#include "../io/bios_drive.h"

void read_block(struct ext2_fs* fs, uint32_t block, void* buffer)
{
    bdrive_read(
        fs->drive_num, 
        fs->block_size / 512, 
        (fs->block_size * block) / 512 + fs->start_lba,
        buffer
    );
}

void ext2_init(uint8_t drive_num, uint32_t start_lba, uint32_t num_sectors)
{
    struct ext2_fs* fs = kcalloc(sizeof(struct ext2_fs));

    fs->drive_num = drive_num;
    fs->start_lba = start_lba;
    fs->sb = kalloc(1024);
    
    // Temp value so we can read
    fs->block_size = 1024;

    // Read in the superblock, and initialise fields from that
    // bdrive_read(drive_num, 2, start_lba + 2, fs->sb);
    read_block(fs, 1, fs->sb);
    fs->block_size = (1 << 10) << fs->sb->log_block_size;
    fs->blocks_per_group = fs->sb->blocks_per_group;
    fs->inodes_per_group = fs->sb->inodes_per_group;

    // total blocks / blocks per group, rounded up gives the number
    // of groups total
    fs->num_groups = fs->sb->total_blocks / fs->blocks_per_group;
    if (fs->blocks_per_group * fs->num_groups < fs->num_groups)
        fs->num_groups++;


    uint32_t bgd_table_blocks = (fs->num_groups * sizeof(struct ext2_bgd));
    if (bgd_table_blocks * fs->block_size < fs->num_groups * sizeof(struct ext2_bgd))
        bgd_table_blocks++;

    debugf("num_groups: %d, bgd_table_blocks: %d", fs->num_groups, bgd_table_blocks);
}