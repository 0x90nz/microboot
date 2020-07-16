#include "crc.h"

#define POLYNOMIAL  0xEDB88320

uint32_t crc32(const void *data, size_t length)
{
    uint8_t* message = (uint8_t*)data;

    int i = 0;
    uint32_t crc = ~0;
    for (int i = 0; message[i] != 0; i++)
    {
        crc = crc ^ message[i];
        for (int j = 7; j >= 0; j--)
        {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
    return ~crc;
}