#include <stdint.h>
#include "pio.h"
#include "serial.h"
#include "../stdlib.h"
#include "../printf.h"
#include "../sys/interrupts.h"
#include "../kernel.h"

static uint16_t sp_port;

static void serial_handle_irq(uint32_t int_no, uint32_t err_no)
{
    printf("serial gotten\n");
}

void serial_init(uint16_t port)
{
    // Register the handlers
    register_handler(IRQ_TO_INTR(4), serial_handle_irq);
    register_handler(IRQ_TO_INTR(3), serial_handle_irq);

    outb(port + 1, 0x00);       // Disable all interrupts
    outb(port + 3, 0x80);       // Set baud rate div.
    outb(port + 0, 0x03);       // Low byte of divisor
    outb(port + 1, 0x00);       // High byte of divisor
    outb(port + 3, 0x03);       // 8-N-1
    outb(port + 2, 0xc7);       // Enable FIFO, clear and set 14-byte thresh
    outb(port + 4, 0x0b);       // Enable IRQs

    sp_port = port;
}

static int serial_tx_empty()
{
    return inb(sp_port + 5) & 0x20;
}

void serial_putc(char c)
{
    while (serial_tx_empty() == 0) { hlt(); }
    outb(sp_port, c);
}
