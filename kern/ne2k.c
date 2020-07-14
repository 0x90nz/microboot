#include "ne2k.h"
#include "pio.h"
#include "kernel.h"
#include "stdlib.h"

uint16_t iobase;

void tx_packet(void* packet, size_t len)
{
    outb(iobase + NE2K_REG_CMD, 0x22); // Start, nodma

    // Load the packet size
    outb(iobase + NE2K_RBCR0, len & 0xff);
    outb(iobase + NE2K_RBCR1, (len >> 8) & 0xff);

    outb(iobase + NE2K_REG_INT_STAT, inb(iobase + NE2K_REG_INT_STAT) | (1 << 6));

    outb(iobase + NE2K_RSAR0, 0x00); // Always 0
    outb(iobase + NE2K_RSAR1, 0x01); // Target page


    outb(iobase + NE2K_REG_CMD, 0x12); // start, remote write DMA

    uint8_t* temp_packet = (uint8_t*)packet;
    for (size_t i = 0; i < len; i++)
    {
        outw(iobase + NE2K_REG_DATA, temp_packet[i]);
    }

    // Wait until bit 6 is set
    while (inb(iobase + NE2K_REG_INT_STAT) & (1 << 6) == 0) { puts("poll\n");};
    puts("done tx\n");
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

    print_hex(prom[0]);
    puts(":");
    print_hex(prom[1]);
    puts(":");
    print_hex(prom[2]);
    puts(":");
    print_hex(prom[3]);
    puts(":");
    print_hex(prom[4]);
    puts(":");
    print_hex(prom[5]);
    puts("\n");

    tx_packet(prom, 32);
}