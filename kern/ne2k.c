#include "ne2k.h"
#include "pio.h"
#include "interrupts.h"
#include "kernel.h"
#include "stdlib.h"
#include "alloc.h"
#include "ip.h"

uint16_t iobase;

int ne2k_tx_packet(void* packet, size_t len)
{
    outb(iobase + NE2K_REG_CMD, 0b00100010);

    // Set the length to transmit
    outb(iobase + NE2K_RBCR0, len & 0xff);
    outb(iobase + NE2K_RBCR1, (len >> 8) & 0xff);

    outb(iobase + NE2K_REG_INT_STAT, 1 << 6);

    // Set which buffer we're going to use
    int txbuf = 0x40;
    outb(iobase + NE2K_RSAR0, 0);
    outb(iobase + NE2K_RSAR1, txbuf);

    outb(iobase + NE2K_REG_CMD, NE2K_CMD_REMOTE_WRITE | NE2K_CMD_START);

    uint8_t* data = (uint8_t*)packet;
    for (size_t i = 0; i < len; i++)
    {
        outb(iobase + NE2K_REG_DATA, data[i]);
    }

    // Wait for the remote DMA to complete
    while ((inb(iobase + NE2K_REG_INT_STAT) & (1 << 6)) == 0);

    outb(iobase + NE2K_REG_TPSR, txbuf);

    outb(iobase + NE2K_REG_TBCR0, len & 0xff);
    outb(iobase + NE2K_REG_TBCR1, (len >> 8) & 0xff);

    // nodma, txp, start
    outb(iobase + NE2K_REG_CMD, 0b110);

    return inb(iobase + NE2K_REG_INT_STAT);
}

void ne2k_handle_irq(uint32_t int_no, uint32_t err_no)
{
    printf("ne2k intr: %08b\n", inb(iobase + NE2K_REG_INT_STAT));
    // For now just acknowledge everything
    outb(iobase + NE2K_REG_INT_STAT, inb(iobase + NE2K_REG_INT_STAT));
}

void ne2k_select_page(int page)
{
    ASSERT(page >= 0 && page <= 4, "Invalid page for ne2k");
    outb(iobase + NE2K_REG_CMD, (inb(iobase + NE2K_REG_CMD) & 0x3f) | (page << 6));
}

void ne2k_init(pci_device_desc_t* device)
{
    // Get rid of the two least significant bits of the iobase
    iobase = device->bar0 & ~0x3;

    outb(iobase + NE2K_REG_RESET, inb(iobase + NE2K_REG_RESET));    // Reset
    while ((inb(iobase + NE2K_REG_INT_STAT) & 0x80) == 0);    // Poll waiting for reset to actually be done
    outb(iobase + NE2K_REG_INT_STAT, 0xff);                  // Mask all interrupts

    outb(iobase, (1 << 5) | 1); // Page 0, no DMA, stop
    
    outb(iobase + NE2K_REG_DATA_CFG, 0x49);
    
    outb(iobase + NE2K_RBCR0, 0);     // Clear count
    outb(iobase + NE2K_RBCR1, 0);

    outb(iobase + NE2K_REG_INT_MASK, 0);     // Mask completion IRQ
    outb(iobase + NE2K_REG_INT_STAT, 0xff);

    outb(iobase + NE2K_REG_RX_CFG, 0x20);  // Monitor mode
    outb(iobase + NE2K_REG_TX_CFG, 0x02);  // Normal

    outb(iobase + NE2K_RBCR0, 32);    // Read 32 bytes
    outb(iobase + NE2K_RBCR1, 0);     // High byte of count

    outb(iobase + NE2K_RSAR0, 0);     // Start DMA at 0
    outb(iobase + NE2K_RSAR1, 0);     // High byte of DMA start

    outb(iobase, NE2K_CMD_REMOTE_READ | NE2K_CMD_START);         // Actually start the Read

    uint8_t prom[32];
    for (int i = 0; i < 32; i++)
    {
        prom[i] = inb(iobase + NE2K_REG_DATA);
    }

    ne2k_select_page(1);

    // Write the MAC addr to par0..6
    for (int i = 0; i < 6; i++)
    {
        outb(iobase + 1 + i, prom[i]);
    }

    printf("mac: %02x:%02x:%02x:%02x:%02x:%02x\n", prom[0], prom[1], prom[2], prom[3], prom[4], prom[5]);

    ne2k_select_page(2);
    uint8_t dcr = inb(NE2K_REG_DATA_CFG);
    ne2k_select_page(0);
    // Blank out the low two bits, leaving us in byte-wise DMA mode
    outb(iobase + NE2K_REG_DATA_CFG, dcr & 0xfc);

    register_handler(IRQ_TO_INTR(device->interrupt_line), ne2k_handle_irq);

    // Enable interrupts for tx and rx
    outb(iobase + NE2K_REG_INT_MASK, 0b11);
}