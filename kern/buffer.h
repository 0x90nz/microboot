#pragma once

#include <stdint.h>
#include <stddef.h>

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
void ringbuffer_get_obj(struct ringbuffer* rbuf, void* out, size_t size);
void ringbuffer_put_obj(struct ringbuffer* rbuf, void* data, size_t size);
int ringbuffer_empty(struct ringbuffer* rbuf);
void ringbuffer_reset(struct ringbuffer* rbuf);