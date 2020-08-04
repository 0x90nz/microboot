#pragma once

#include <stdint.h>

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
};

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
};

enum ext2_error_mode {
    EXT2_ERR_IGNORE = 1,
    EXT2_ERR_REMOUNT_RO = 2,
    EXT2_ERR_PANIC = 3,
};

enum ext2_state {
    EXT2_CLEAN = 1,
    EXT2_ERRORS = 2,
};

