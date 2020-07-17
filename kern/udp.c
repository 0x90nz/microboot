#include "udp.h"

void* udp_make_packet(void* packet, size_t data_length, uint16_t src_port, uint16_t dst_port)
{
    udp_header_t* header = packet;
    header->src_port = src_port;
    header->dst_port = dst_port;
    header->length = data_length + sizeof(udp_header_t);
    header->cksum = 0; // Checksum is optional, so we ignore it
    return packet + sizeof(udp_header_t);
}

size_t udp_buffer_length(size_t data_length)
{
    return data_length + sizeof(udp_header_t);
}