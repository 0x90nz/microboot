#include "ether.h"
#include "../stdlib.h"

void* ether_make_packet(void* buffer, uint8_t* src, uint8_t* dst, uint16_t length)
{
    ASSERT(length <= 1500, "Packet too large for ethernet");
    ether_header_t* header = (ether_header_t*)buffer;
    memcpy(header->src_mac, src, 6);
    memcpy(header->dst_mac, dst, 6);
    header->ethertype = 0x0008;
    return buffer + sizeof(ether_header_t);
}

size_t ether_buffer_length(size_t data_size)
{
    // Data + header + crc
    return (data_size > 48 ? data_size : 48) + sizeof(ether_header_t) + 4;
}

void ether_calc_crc(void* buffer, size_t data_size)
{

}