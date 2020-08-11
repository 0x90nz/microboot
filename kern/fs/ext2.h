#pragma once

#include <stdint.h>

// Extended information. Missing journal support
struct ext2_extended {
    uint32_t first_inode;
    uint16_t inode_size;
    uint16_t block_group_num;
    uint32_t feature_set;
    uint32_t incompat_feature_set;
    uint32_t ro_feature_set;
    uint8_t uuid[16];
    char vol_name[16];
    char last_mounted[64];
    uint32_t usage_bitmap;

    uint8_t num_blocks_prealloc;
    uint8_t num_dir_blocks_prealloc;
    uint16_t reserved0;
} __attribute__((packed));

struct ext2_superblock {
    uint32_t total_inodes;
    uint32_t total_blocks;
    uint32_t resvd_blocks;
    uint32_t free_blocks;
    uint32_t free_inodes;
    uint32_t first_data_block;
    uint32_t log_block_size;
    uint32_t log_frag_size;
    uint32_t blocks_per_group;
    uint32_t frags_per_group;
    uint32_t inodes_per_group;
    uint32_t mount_time;
    uint32_t write_time;
    uint16_t mount_count;
    uint16_t max_mount_count;
    uint16_t magic;
    uint16_t state;
    uint16_t error_behaviour;
    uint16_t minor_rev_level;
    uint32_t last_check_time;
    uint32_t check_interval;
    uint32_t creator_os;
    uint32_t revision_level;
    uint16_t default_uid;
    uint16_t default_gid;
    struct ext2_extended ext;
} __attribute__((packed));

enum ext2_error_mode {
    EXT2_ERR_IGNORE = 1,
    EXT2_ERR_REMOUNT_RO = 2,
    EXT2_ERR_PANIC = 3,
};

enum ext2_state {
    EXT2_CLEAN = 1,
    EXT2_ERRORS = 2,
};

struct ext2_inode {
    uint16_t type_and_perms;
    uint16_t uid;
    uint32_t size_lo;
    uint32_t last_access_time;
    uint32_t creation_time;
    uint32_t last_modification_time;
    uint32_t deletion_time;
    uint16_t gid;
    uint16_t hardlink_count;
    uint32_t sectors_used;
    uint32_t flags;
    uint32_t osv1;
    uint32_t direct_ptr[12];
    uint32_t singly_indirect_ptr;
    uint32_t doubly_indirect_ptr;
    uint32_t triply_indirect_ptr;
    uint32_t generation_num;
    uint32_t v1_xattrs;
    uint32_t v1_size_hi_acl;
    uint32_t frag_block_addr;
    uint8_t osv2[12];
} __attribute__((packed));

enum inode_type {
    EXT2_INODE_FIFO = 0x1000,
    EXT2_INODE_CHAR = 0x2000,
    EXT2_INODE_DIR  = 0x4000,
    EXT2_INODE_BLK  = 0x6000,
    EXT2_INODE_REG  = 0x8000,
    EXT2_INODE_SYML = 0xA000,
    EXT2_INODE_SOCK = 0xC000,
};

struct ext2_dir_entry {
    uint32_t inode;
    uint16_t size;
    uint8_t name_length;
    uint8_t type_indicator;
    char first_name_char;
} __attribute__((packed));

struct ext2_bgd {
    uint32_t addr_block_usage_bitmap;
    uint32_t addr_inode_usage_bitmap;
    uint32_t addr_block_inode_table;
    uint16_t free_blocks;
    uint16_t free_inodes;
    uint16_t directories;
} __attribute__((packed));

/**
 * OS accounting information for this file system. Is NOT an ext2 structure
 * that would be found on disk
 */
struct ext2_fs {
    uint8_t drive_num;
    uint32_t start_lba;
    uint32_t block_size;
    uint32_t num_groups;
    uint32_t blocks_per_group;
    uint32_t inodes_per_group;
    struct ext2_superblock* sb;
    struct ext2_bgd* bgd_table;
};

void ext2_init(uint8_t drive_num, uint32_t start_lba, uint32_t num_sectors);
