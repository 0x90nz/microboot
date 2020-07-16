#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint8_t version     : 4;
    uint8_t ihl         : 4;
    uint8_t dscp        : 6;
    uint8_t ecn         : 2;
    uint16_t length;

    uint16_t identification;
    uint8_t flags       : 3;
    uint16_t fragoff    : 13;

    uint8_t ttl;
    uint8_t protocol;
    uint16_t header_cksum;

    uint32_t src_ip;
    uint32_t dst_ip;
} __attribute__((packed, scalar_storage_order("big-endian"))) ipv4_header_t;

typedef struct {
    uint8_t addr[4];
} ip_addr_t;

// Takes a raw buffer and returns the address at which payload data may be added,
// as well as initialising the IPv4 header
void* ip_make_packet(void* packet, size_t data_length, uint8_t protocol, uint32_t src, uint32_t dst);

// Returns the size required for a buffer given a data length
size_t ip_buffer_length(size_t data_length);