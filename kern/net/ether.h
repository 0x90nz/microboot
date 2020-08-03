#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint8_t dst_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype;
} __attribute__((packed)) ether_header_t;

void* ether_make_packet(void* buffer, uint8_t* src, uint8_t* dst, uint16_t length);
size_t ether_buffer_length(size_t data_size);
void ether_calc_crc(void* buffer, size_t data_size);