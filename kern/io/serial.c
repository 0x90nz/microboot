#include <stdint.h>
#include "pio.h"
#include "serial.h"
#include "../buffer.h"
#include "../stdlib.h"
#include "../printf.h"
#include "../sys/interrupts.h"
#include "../kernel.h"

static uint16_t sp_port;
static struct ringbuffer in_buffer;
static struct ringbuffer out_buffer;

#define SP_BUFFER_SIZE 32
static uint8_t in_base_buffer[SP_BUFFER_SIZE];
static uint8_t out_base_buffer[SP_BUFFER_SIZE];

static int force_tx = 0;
static int setup = 0;

static void serial_handle_irq(uint32_t int_no, uint32_t err_no)
{
    uint8_t id = inb(sp_port + 2);

    // Change in the line status register
    uint8_t lsr = -1;
    if ((id & 0x07) == 0b110) {
        lsr = inb(sp_port + 5);
    }

    // Recieved data available
    if ((id & 0x07) == 0b0100) {
        char c = inb(sp_port);
        ringbuffer_put(&in_buffer, c);
    }
    
    // THR empty
    if ((id & 0x07) == 0b0010) {
        // THR is empty, so we can send stuff
        if (!ringbuffer_empty(&out_buffer)) {
            outb(sp_port, ringbuffer_get(&out_buffer));
        }
    }

    /**
     * This is convoluted, and will probably be replaced at some point once I
     * figure out a nicer way to do it. In essence, we need to be able to kick
     * off the transfer, so we force transmission of something. From there we
     * can easily just use the THR empty interrupt, but not so for the first time.
     */
    if (force_tx) {
        lsr = lsr != -1 ? lsr : inb(sp_port + 5);
        if (lsr & 0x20) {
            if (!ringbuffer_empty(&out_buffer)) {
                outb(sp_port, ringbuffer_get(&out_buffer));
            }
        }
	force_tx = 0;
    }
}

void serial_init(uint16_t port)
{
    // Setup ring buffers
    ringbuffer_init(&in_buffer, in_base_buffer, SP_BUFFER_SIZE);
    ringbuffer_init(&out_buffer, out_base_buffer, SP_BUFFER_SIZE);

    sp_port = port;

    // Register the handlers
    register_handler(IRQ_TO_INTR(4), serial_handle_irq);

    outb(port + 1, 0x00);       // Disable all interrupts
    outb(port + 3, 0x80);       // Set baud rate div.
    outb(port + 0, 0x03);       // Low byte of divisor
    outb(port + 1, 0x00);       // High byte of divisor
    outb(port + 3, 0x03);       // 8-N-1
    outb(port + 2, 0xc7);       // Enable FIFO, clear and set 14-byte thresh
    outb(port + 4, 0x0b);       // Enable IRQs
    outb(port + 1, 0x07);

    setup = 1;
}

void serial_clear_input()
{
    ringbuffer_reset(&in_buffer);
}

int serial_available()
{
    return !ringbuffer_empty(&in_buffer);
}

static inline int serial_tx_empty()
{
    return inb(sp_port + 5) & 0x20;
}

void serial_putc(char c)
{
    if (setup) {
        ringbuffer_put(&out_buffer, c);
        // Ugly hack. See `serial_handle_irq` for details. Replace at some point
        asm("int $36");
        force_tx = 1;
    }
}

char serial_getc()
{
    while (ringbuffer_empty(&in_buffer)) { hlt(); }

    return ringbuffer_get(&in_buffer);
}
