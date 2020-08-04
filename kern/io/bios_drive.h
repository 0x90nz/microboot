#pragma once

#include <stdint.h>
#include <stddef.h>

void bdrive_init();
void bdrive_read_sectors(
    uint8_t drive_number, 
    uint32_t cyl, uint32_t heads, uint32_t sect, 
    uint8_t* buffer, uint8_t num_sectors
);