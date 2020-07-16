#include "ether.h"

void* ether_make_packet(void* buffer)
{

}

size_t ether_buffer_length(size_t data_size)
{
    // Data + header + crc
    return data_size + sizeof(ether_header_t) + 4;
}

void ether_calc_crc(void* buffer, size_t data_size)
{

}