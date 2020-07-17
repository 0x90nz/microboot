#include "ip.h"

void* ip_make_packet(void* packet, size_t data_length, uint8_t protocol, uint32_t src, uint32_t dst)
{
    ipv4_header_t* header = packet;
    header->version = 4;
    header->ihl = 5;    // Minimum length, we use no other options yet

    header->dscp = 0;
    header->ecn = 0;

    header->length = sizeof(ipv4_header_t) + data_length;
    header->identification = 0;
    
    header->flags = 0b000;
    header->fragoff = 0;

    header->ttl = 128;
    header->protocol = protocol;
    header->src_ip = src;
    header->dst_ip = dst;
    return packet + sizeof(ipv4_header_t);
}

size_t ip_buffer_length(size_t data_length)
{
    return data_length + sizeof(ipv4_header_t);
}

uint32_t make_ip(int o1, int o2, int o3, int o4)
{
    return ((o1 & 0xff) << 24) | ((o2 & 0xff) << 16) | ((o3 & 0xff) << 8) | (o4 & 0xff);
}