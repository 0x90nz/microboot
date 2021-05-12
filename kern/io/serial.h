#pragma once

#include <stdint.h>
#include "../buffer.h"
#include "chardev.h"

#define SP_COM0_PORT        0x3f8
#define SP_COM1_PORT        0x2f8
#define SP_COM2_PORT        0x3e8
#define SP_COM4_PORT        0x2e8

#define SP_BUFFER_SIZE 32

struct serial_port {
    uint16_t iobase;
};

void serial_init(struct serial_port* port, uint16_t iobase, uint32_t baudrate);
void serial_get_chardev(struct serial_port* port, chardev_t* chardev);