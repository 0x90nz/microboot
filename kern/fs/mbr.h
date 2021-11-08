#pragma once

#include <stdint.h>

struct mbr_part_entry {
    uint8_t physical_drive;
    uint8_t start_cyl, start_head, start_sector;
    uint8_t type;
    uint8_t end_cyl, end_head, end_sector;
    uint32_t start_lba;
    uint32_t num_sectors;
} __attribute__((packed));

struct mbr_sector {
    uint8_t bootstrap[446];
    struct mbr_part_entry partitions[4];
    uint16_t active_flag;
} __attribute__((packed));
