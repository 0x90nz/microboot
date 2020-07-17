#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t cksum;
} __attribute__((packed, scalar_storage_order("big-endian"))) udp_header_t;

// Takes a raw buffer and returns the address at which payload data may be added,
void* udp_make_packet(void* packet, size_t data_length, uint16_t src_port, uint16_t dst_port);

// Returns the size required for a buffer given a data length
size_t udp_buffer_length(size_t data_length);