#include <stdint.h>
#include "pio.h"
#include "serial.h"
#include "../stdlib.h"
#include "../kernel.h"


void serial_init(struct serial_port* port, uint16_t iobase, uint32_t baudrate)
{
    // Setup ring buffers
    port->iobase = iobase;

    uint16_t divisor = 115200 / baudrate;

    outb(port->iobase + 1, 0x00);               // Disable all interrupts
    outb(port->iobase + 3, 0x80);               // Set baud rate div.
    outb(port->iobase + 0, divisor & 0xff);     // Low byte of divisor
    outb(port->iobase + 1, divisor >> 8);       // High byte of divisor
    outb(port->iobase + 3, 0x03);               // 8-N-1
}

static inline int serial_tx_empty(uint16_t iobase)
{
    return inb(iobase + 5) & 0x20;
}

static inline int serial_available(uint16_t iobase)
{
    return inb(iobase + 5) & 1;
}

void serial_putc(struct serial_port* port, char c)
{
    while (!serial_tx_empty(port->iobase));
    outb(port->iobase, c);
}

char serial_getc(struct serial_port* port)
{
    while (!serial_available(port->iobase));
    return inb(port->iobase);
}

static int chardev_putc(chardev_t* dev, int c)
{
    struct serial_port* port = (struct serial_port*)dev->priv;
    serial_putc(port, c);
    return 1;
}

static int chardev_getc(chardev_t* dev)
{
    struct serial_port* port = (struct serial_port*)dev->priv;
    return serial_getc(port);
}

void serial_get_chardev(struct serial_port* port, chardev_t* chardev)
{
    chardev->putc = chardev_putc;
    chardev->getc = chardev_getc;
    chardev->priv = port;
}

