#pragma once

#include <stdint.h>

struct mbr_part_entry {
    uint8_t drive_attributes;
    uint8_t start_cyl, start_head, start_sector;
    uint32_t start_lba;
    uint32_t num_sectors;
} __attribute__((packed));

struct mbr_sector {
    uint8_t bootstrap[450];
    struct mbr_part_entry partitions[4];
    uint16_t active_flag;
} __attribute__((packed));