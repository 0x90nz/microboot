#include <stdint.h>
#include <export.h>
#include "pio.h"
#include "serial.h"
#include "../alloc.h"
#include "../stdlib.h"
#include "../kernel.h"

#define SP_COM0_PORT        0x3f8
#define SP_COM1_PORT        0x2f8
#define SP_COM2_PORT        0x3e8
#define SP_COM3_PORT        0x2e8

#define SP_BUFFER_SIZE 32

struct serial_port {
    uint16_t iobase;
    uint32_t baudrate;
};

static inline int serial_tx_empty(uint16_t iobase)
{
    return inb(iobase + 5) & 0x20;
}

static inline int serial_available(uint16_t iobase)
{
    return inb(iobase + 5) & 1;
}

static void serial_putc(struct serial_port* port, char c)
{
    while (!serial_tx_empty(port->iobase));
    outb(port->iobase, c);
}

static char serial_getc(struct serial_port* port)
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

static void serial_get_chardev(struct serial_port* port, chardev_t* chardev)
{
    chardev->putc = chardev_putc;
    chardev->getc = chardev_getc;
    chardev->priv = port;
}

static void serial_destroy(struct device* dev)
{
    kfree(dev->device_priv);
    kfree(dev);
}

static struct device* serial_new_dev(uint16_t iobase, uint32_t baudrate)
{
    static int sp_index = 0;

    struct serial_port* sp = kalloc(sizeof(*sp));
    sp->iobase = iobase;
    sp->baudrate = baudrate;

    struct device* dev = kalloc(sizeof(*dev));
    dev->type = DEVICE_TYPE_CHAR;
    dev->destroy = serial_destroy;
    sprintf(dev->name, "sp%d", sp_index++);
    dev->device_priv = sp;

    dev->internal_dev = kalloc(sizeof(chardev_t));
    serial_get_chardev(sp, dev->internal_dev);

    return dev;
}

static void serial_probe(struct driver* driver)
{
    if (!driver->first_probe)
        return;

    device_register(serial_new_dev(SP_COM0_PORT, 115200));
    device_register(serial_new_dev(SP_COM1_PORT, 115200));
    device_register(serial_new_dev(SP_COM2_PORT, 115200));
    device_register(serial_new_dev(SP_COM3_PORT, 115200));
}

struct driver serial_driver = {
    .name = "PIO serial",
    .probe = serial_probe,
    .driver_priv = NULL
};

static void serial_register_driver()
{
    driver_register(&serial_driver);
}
EXPORT_INIT(serial_register_driver);

