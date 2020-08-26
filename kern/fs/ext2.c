/**
 * @file ext2.c
 * @brief Read-only support for an ext2 filesystem
 */

#include "ext2.h"
#include "../stdlib.h"
#include "../alloc.h"
#include "../io/bios_drive.h"

#ifdef EXT2_ENABLE

/**
 * @brief Read a single block from an ext2 filesystem
 * 
 * @param fs the filesystem
 * @param block the block number
 * @param buffer the buffer to read into
 */
void read_block(struct ext2_fs* fs, uint32_t block, void* buffer)
{
    uint32_t sectors_per_block = fs->block_size / 512;
    bdrive_read(
        fs->drive_num, 
        sectors_per_block, 
        (sectors_per_block * block) + fs->start_lba,
        buffer
    );
}

/**
 * @brief Read the inode metadata for a given inode index, 1 indexed
 * 
 * @param fs the filesystem
 * @param inode the inode to read data into
 * @param inode_num the inode number to read from
 */
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

/**
 * This mass of code determines where the iblock is located (i.e. the actual
 * index of the block as it is on disk). Note: triply indirect references will
 * cause 3 reads! This could get slow if used too often
 */ 
static uint32_t disk_block_num(struct ext2_fs* fs, struct ext2_inode* inode, uint32_t iblock)
{
    uint32_t* tmp = kalloc(fs->block_size);
    uint32_t ret = -1;
    uint32_t u32_blksz = fs->block_size / 4;

    if (iblock <= 11) {
        ret = inode->direct_ptr[iblock];
        goto cleanup;
    }

    uint32_t blocks_left = iblock - 12;

    // Singly indirect pointer
    if (blocks_left < u32_blksz) {
        read_block(fs, inode->singly_indirect_ptr, tmp);
        ret = tmp[blocks_left];
        goto cleanup;
    }

    // Doubly indirect pointer
    blocks_left -= u32_blksz;

    if (blocks_left < u32_blksz * u32_blksz) {
        uint32_t d_index = blocks_left / u32_blksz;
        uint32_t index = blocks_left - d_index * u32_blksz;

        read_block(fs, inode->doubly_indirect_ptr, tmp);
        read_block(fs, tmp[d_index], tmp);
        ret = tmp[index];
        goto cleanup;
    }

    blocks_left -= u32_blksz * u32_blksz;
    if (blocks_left < u32_blksz * u32_blksz * u32_blksz) {
        ASSERT(0, "Not yet implemented");
    }

    ASSERT(0, "Invalid block pointer");

    // Oh no, a goto! https://xkcd.com/292/
cleanup:
    kfree(tmp);
    return ret;
}

/**
 * @brief Read data from a given inode
 * 
 * @param fs the filesystem
 * @param inode the inode to read from
 * @param offset the offset within the data to read from
 * @param size the size of data to read
 * @param buffer the buffer to read into
 * @return uint32_t the number of bytes read
 */
uint32_t read_inode_data(struct ext2_fs* fs, struct ext2_inode* inode, uint32_t offset, uint32_t size, void* buffer)
{
    // Limit the end offset we want to read to the length of the inode if it
    // goes over the actual length of the file. Caller can determine if this
    // happened by looking at the return value
    uint32_t end_off = (inode->size_lo >= offset + size) ? (offset + size) : inode->size_lo;
    
    uint32_t start_block = offset / fs->block_size;
    uint32_t end_block = end_off / fs->block_size;

    // The offset into the first block that we need to start reading from
    uint32_t start_off = offset % fs->block_size;
    // The amount that we need to read from the final block
    uint32_t end_block_size = end_off - end_block * fs->block_size;

    uint32_t i = start_block;
    uint32_t curr_off = 0;

    char* buf = kalloc(fs->block_size);
    while (i <= end_block) {
        uint32_t start = 0, end = fs->block_size - 1;

        uint32_t dsk_block = disk_block_num(fs, inode, i);

        ASSERT(dsk_block != -1, "Invalid disk block");
        
        read_block(fs, dsk_block, buf);

        if (i == start_block)
            start = start_off;
        if (i == end_block)
            end = end_block_size - 1;

        memcpy(buffer + curr_off, buf + start, (end - start + 1));
        curr_off += (end - start + 1);
        i++;
    }

    kfree(buf);
    return end_off - offset;
}

/**
 * @brief List a directory
 * 
 * @param fs the filesystem
 * @param inode the directory inode
 */
void ext2_listdir(struct ext2_fs* fs, struct ext2_inode* inode)
{
    struct ext2_dir_entry* dirent = kalloc(inode->size_lo);
    read_inode_data(fs, inode, 0, inode->size_lo, dirent);

    struct ext2_dir_entry* current = dirent;
    uint32_t size = 0;
    while (size < inode->size_lo) {
        printf("%s\n", &current->first_name_char);
        size += current->size;
        current =  (void*)current + current->size;        
    }

    kfree(dirent);
}

/**
 * @brief get the inode number of a file within a directory
 * 
 * @param fs the filesystem
 * @param inode the inode number of the directory
 * @param name the name of the file
 * @return uint32_t the inode number of the file within the directory
 */
uint32_t get_dir(struct ext2_fs* fs, struct ext2_inode* inode, const char* name)
{
    struct ext2_dir_entry* dirent = kalloc(inode->size_lo);
    read_inode_data(fs, inode, 0, inode->size_lo, dirent);

    struct ext2_dir_entry* current = dirent;
    uint32_t num = -1;
    uint32_t size = 0;
    while (size < inode->size_lo) {
        if (strcmp(name, &current->first_name_char) == 0) {
            num = current->inode;
            break;
        }
        size += current->size;
        current =  (void*)current + current->size;        
    }

    kfree(dirent);
    return num;
}

uint32_t ext2_find_file(struct ext2_fs* fs, const char* fname, struct ext2_inode* inode)
{

}

/**
 * @brief Initialise the ext2 filesystem
 * 
 * @param drive_num the bios drive number (e.g. 0x80 for the first disk)
 * @param start_lba the start lba of the ext2 partition
 * @param num_sectors the number of sectors for the whole filesystem
 * @return struct ext2_fs* a pointer to filesystem info
 */
struct ext2_fs* ext2_init(uint8_t drive_num, uint32_t start_lba, uint32_t num_sectors)
{
    struct ext2_fs* fs = kallocz(sizeof(struct ext2_fs));

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
    fs->bgd_table = kallocz(bgd_table_blocks * fs->block_size * sizeof(struct ext2_bgd));
    for (uint32_t i = 0; i < bgd_table_blocks; i++) {
        read_block(fs, 2, (void*)fs->bgd_table + i * fs->block_size);
    }

    fs->root_inode = kallocz(sizeof(struct ext2_inode));
    read_inode(fs, fs->root_inode, 2);

    return fs;
}

#endif