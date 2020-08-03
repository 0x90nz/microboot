#include "ioutil.h"

void fill_buffer(char (*get)(), char* buffer, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        buffer[i] = get();
    }
}