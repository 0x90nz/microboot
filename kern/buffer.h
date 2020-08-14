#pragma once

#include <stdint.h>

struct ringbuffer {
    uint8_t* buffer;
    uint32_t head;
    uint32_t tail;
    uint32_t size;
    int full;
};

void ringbuffer_init(struct ringbuffer* rbuf, uint8_t* buffer, uint32_t max_size);
uint8_t ringbuffer_get(struct ringbuffer* rbuf);
void ringbuffer_put(struct ringbuffer* rbuf, uint8_t data);
int ringbuffer_empty(struct ringbuffer* rbuf);
void ringbuffer_reset(struct ringbuffer* rbuf);