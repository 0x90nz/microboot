#pragma once

#include <stdint.h>
#include <stddef.h>

struct disk_addr {
    uint8_t size;
    uint8_t zero;
    uint16_t num_sectors;
    uint32_t buffer;
    uint32_t low_lba;
    uint32_t high_lba;
} __attribute__((packed));

// void bdrive_read(uint8_t drive_num, uint16_t num_sectors, uint64_t lba, void* buffer);
