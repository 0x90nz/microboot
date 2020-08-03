#include "ata.h"
#include "pio.h"
#include "../stdlib.h"

void select_drive(uint8_t bus, uint8_t drive)
{
    if(bus == ATA_PRIMARY)
    {
        if(drive == ATA_MASTER)
            outb(ATA_IOBASE + REG_IO_DEVSEL, 0xa0);
        else
            outb(ATA_IOBASE + REG_IO_DEVSEL, 0xb0);
    }
    else
    {
        if(drive == ATA_SLAVE)
        {
            if(drive == ATA_MASTER)
                outb(ATA_SEC_IOBASE + REG_IO_DEVSEL, 0xa0);
            else
                outb(ATA_SEC_IOBASE + REG_IO_DEVSEL, 0xb0);
        }
    }
}

static void ata_delay()
{
    for(int i = 0; i < 4; i++)
        inb(ATA_CONTROL_BASE + REG_CTRL_ALTSTATUS);
}

static void ata_reset()
{
    outb(ATA_CONTROL_BASE + 0, REG_CTRL_SOFTRESET);
    ata_delay();
    outb(ATA_CONTROL_BASE + 0, 0x00);
}

void ata_init()
{
    ata_reset();
    // select_drive(bus, drive);
}

uint8_t identify()
{
    int bus = 0;
    uint16_t io_port = 0;
    // select_drive(bus, drive);
    if(bus == ATA_PRIMARY)
        io_port = ATA_IOBASE;
    else
        io_port = ATA_SEC_IOBASE;

    outb(io_port + REG_IO_SECCOUNT, 0);
    outb(io_port + REG_IO_LBA0, 0);
    outb(io_port + REG_IO_LBA1, 0);
    outb(io_port + REG_IO_LBA2, 0);

    outb(io_port + REG_IO_COMMAND, 0xec); // IDENTIFY command


    uint8_t status = inb(io_port + REG_IO_STATUS);
    if(status)
    {
        while((inb(io_port + REG_IO_STATUS) & STATUS_BSY) != 0)
        {
            status = inb(io_port + REG_IO_STATUS);
        }

        while(!(status & STATUS_DRQ))
        {
            if(status & STATUS_ERR)
            {
                logf(LOG_WARN, "ATA ERR status on port %04x", io_port);
                return -1;
            }
            status = inb(io_port + REG_IO_STATUS);
        }

        uint16_t buffer[256];
        memset(buffer, 0, 512);
        for(int i = 0; i < 256; i++)
        {
            buffer[i] = inw(ATA_IOBASE + 0);
        }

        // If the drive supports LBA, we say that it exists
        return (buffer[49] & (1 << 9)) != 0 ? 0 : -1;
    }

    return -1;
}