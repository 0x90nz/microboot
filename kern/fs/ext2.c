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

void read_inode(struct ext2_fs* fs, struct ext2_inode* inode, uint32_t inode_num)
{
    // The group to which this inode belongs
    uint32_t group = inode_num / fs->inodes_per_group;
    // The block which points to the inode table
    uint32_t inode_tbl_block = fs->bgd_table[group].addr_block_inode_table;
    uint32_t group_idx = inode_num - (group * fs->inodes_per_group);
    
    // TODO: either account for extended properties not being present, or just
    // throw a tantrum if they're not present
    uint32_t block_off = (group_idx - 1) * fs->sb->ext.inode_size / fs->block_size;

    // The offset _within_ the block that we're going to read
    uint32_t internal_offset = (group_idx - 1) 
            - block_off * (fs->block_size / fs->sb->ext.inode_size);

    debugf("group: %d, tbl_block: %d, group_idx: %d, block_off: %d, internal_off: %d",
        group, inode_tbl_block, group_idx, block_off, internal_offset
    );

    char* buffer = kalloc(fs->block_size);
    read_block(fs, inode_tbl_block + block_off, buffer);
    memcpy(inode, buffer + internal_offset * fs->sb->ext.inode_size, fs->sb->ext.inode_size);
    kfree(buffer);
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

    // Find the number of fs blocks that we need to read to get the BGD table
    uint32_t bgd_table_blocks = (fs->num_groups * sizeof(struct ext2_bgd)) / fs->block_size;
    if (bgd_table_blocks * fs->block_size < fs->num_groups * sizeof(struct ext2_bgd))
        bgd_table_blocks++;

    debugf("num_groups: %d, bgd_table_blocks: %d", fs->num_groups, bgd_table_blocks);
    
    // Allocate space for the BGD table, and then fill it with data from disk
    fs->bgd_table = kcalloc(bgd_table_blocks * fs->block_size * sizeof(struct ext2_bgd));
    for (uint32_t i = 0; i < bgd_table_blocks; i++) {
        read_block(fs, 2, (void*)fs->bgd_table + i * fs->block_size);
    }

    struct ext2_inode* root_inode = kcalloc(sizeof(struct ext2_inode));
    read_inode(fs, root_inode, 2);

    debugf("last access time: %d", root_inode->last_access_time);
}