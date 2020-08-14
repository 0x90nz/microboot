#include "buffer.h"
#include "stdlib.h"

void ringbuffer_init(struct ringbuffer* rbuf, uint8_t* buffer, uint32_t max_size)
{
    ASSERT(rbuf && buffer, "Trying to initialise null buffer");

    rbuf->buffer = buffer;
    rbuf->size = max_size;
    rbuf->head = 0;
    rbuf->tail = 0;
    rbuf->full = 0;
}

void ringbuffer_reset(struct ringbuffer* rbuf)
{
    ASSERT(rbuf, "Can't reset null buffer");

    rbuf->head = 0;
    rbuf->tail = 0;
    rbuf->full = 0;
}

int ringbuffer_empty(struct ringbuffer* rbuf)
{
    return (rbuf->head == rbuf->tail) && !rbuf->full;
}

static void increment_ptr(struct ringbuffer* rbuf)
{
    ASSERT(rbuf, "Can't increment null buffer");

    if (rbuf->full) {
        rbuf->tail = (rbuf->tail + 1) % rbuf->size;
    }

    rbuf->head = (rbuf->head + 1) % rbuf->size;
    rbuf->full = rbuf->head == rbuf->tail;
}

static void decrement_ptr(struct ringbuffer* rbuf)
{
    ASSERT(rbuf, "Can't decrement null buffer");

    rbuf->full = 0;
    rbuf->tail = (rbuf->tail + 1) % rbuf->size;
}

void ringbuffer_put(struct ringbuffer* rbuf, uint8_t data)
{
    ASSERT(rbuf && rbuf->buffer, "Trying to put to invalid buffer");

    rbuf->buffer[rbuf->head] = data;
    increment_ptr(rbuf);
}

uint8_t ringbuffer_get(struct ringbuffer* rbuf)
{
    ASSERT(rbuf && rbuf->buffer, "Trying to get from invalid buffer");
    ASSERT(!ringbuffer_empty(rbuf), "Cannot get from empty buffer");

    uint8_t data = rbuf->buffer[rbuf->tail];
    decrement_ptr(rbuf);
    return data;
}