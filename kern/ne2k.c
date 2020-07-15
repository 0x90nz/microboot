#include "ne2k.h"
#include "pio.h"
#include "interrupts.h"
#include "kernel.h"
#include "stdlib.h"
#include "alloc.h"

uint16_t iobase;

void tx_packet(void* packet, size_t len)
{
    ASSERT(len % 2 == 0, "Cannot tx packet with non-word length");

    outb(iobase + NE2K_RBCR0, len & 0xff);
    outb(iobase + NE2K_RBCR1, (len >> 8) & 0xff);

    int txbuf = 0;
    outb(iobase + NE2K_RSAR0, 0);
    outb(iobase + NE2K_RSAR1, txbuf);

    outb(iobase + NE2K_REG_CMD, NE2K_CMD_REMOTE_WRITE | 0b10);

    uint16_t* data = (uint16_t*)packet;
    for (size_t i = 0; i < len / 2; i++)
    {
        outw(iobase + NE2K_REG_DATA, data[i]);
    }

    outb(iobase + NE2K_REG_TPSR, txbuf);

    outb(iobase + NE2K_REG_TBCR0, len & 0xff);
    outb(iobase + NE2K_REG_TBCR1, (len >> 8) & 0xff);

    // nodma, txp, start
    outb(iobase + NE2K_REG_CMD, 0b110);
}

void ne2k_handle_irq(uint32_t int_no, uint32_t err_no)
{
    puts("ne2k_intr\n");
}

void ne2k_init(pci_device_desc_t* device)
{
    // Get rid of the two least significant bits of the iobase
    iobase = device->bar0 & ~0x3;

    outb(iobase + NE2K_REG_RESET, inb(iobase + NE2K_REG_RESET));    // Reset
    while ((inb(iobase + NE2K_REG_INT_STAT) & 0x80) == 0);    // Poll waiting for reset to actually be done
    outb(iobase + NE2K_REG_INT_STAT, 0xff);                  // Mask all interrupts

    outb(iobase, (1 << 5) | 1); // Page 0, no DMA, stop
    
    outb(iobase + NE2K_REG_DATA_CFG, 0x49);  // Word wide access
    
    outb(iobase + NE2K_RBCR0, 0);     // Clear count
    outb(iobase + NE2K_RBCR1, 0);

    outb(iobase + NE2K_REG_INT_MASK, 0);     // Mask completion IRQ
    outb(iobase + NE2K_REG_INT_STAT, 0xff);

    outb(iobase + NE2K_REG_RX_CFG, 0x20);  // Monitor mode
    outb(iobase + NE2K_REG_TX_CFG, 0x02);  // Loopback

    outb(iobase + NE2K_RBCR0, 32);    // Read 32 bytes
    outb(iobase + NE2K_RBCR1, 0);     // High byte of count

    outb(iobase + NE2K_RSAR0, 0);     // Start DMA at 0
    outb(iobase + NE2K_RSAR1, 0);     // High byte of DMA start

    outb(iobase, NE2K_RBCR0);         // Actually start the Read

    uint8_t prom[32];
    for (int i = 0; i < 32; i++)
    {
        prom[i] = inb(iobase + 0x10);
    }

    // Select page 1
    outb(iobase + NE2K_REG_CMD, (inb(iobase + NE2K_REG_CMD) & 0x3f) | (1 << 6));

    // Write the MAC addr to par0..6
    for (int i = 0; i < 6; i++)
    {
        outb(iobase + 1 + i, prom[i]);
    }

    // Select page 0
    outb(iobase + NE2K_REG_CMD, (inb(iobase + NE2K_REG_CMD) & 0x3f) | (0 << 6));

    register_handler(IRQ_TO_INTR(device->interrupt_line), ne2k_handle_irq);

    char* test = kalloc(1024);
    memset(test, 0xaa, 1024);
    tx_packet(test, 1024);
}