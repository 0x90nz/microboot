#include "bios_drive.h"
#include <stdint.h>
#include <export.h>
#include "../sys/bios.h"
#include "../stdlib.h"
#include "../sys/addressing.h"
#include "../alloc.h"
#include "driver.h"

extern struct disk_addr low_mem_disk_addr;

struct bdrive_priv {
    uint8_t drive_nr;
};

/*
void bdrive_read(uint8_t drive_num, uint16_t num_sectors, uint64_t lba, void* buffer)
{
    ASSERT(num_sectors * 512 <= 4096, "Size would overflow low memory buffer");

    memset(&low_mem_disk_addr, 0, sizeof(struct disk_addr));
    low_mem_disk_addr.size = 16;
    low_mem_disk_addr.num_sectors = num_sectors;
    low_mem_disk_addr.low_lba = lba & 0xffffffff;
    low_mem_disk_addr.high_lba = lba >> 32;
    low_mem_disk_addr.buffer = (uint32_t)&low_mem_buffer;

    struct int_regs regs;
    memset(&regs, 0, sizeof(struct int_regs));

    regs.eax = 0x4200;

    regs.esi = OFFOF((uint32_t)&low_mem_disk_addr);
    regs.ds = SEGOF((uint32_t)&low_mem_disk_addr);

    regs.edx = drive_num & 0xff;

    // debugf("addr of struct %04x:%04x", SEGOF((uint32_t)&low_mem_disk_addr), OFFOF((uint32_t)&low_mem_disk_addr));
    // debugf("addr of low buffer: %04x:%04x", SEGOF((uint32_t)&low_mem_buffer), OFFOF((uint32_t)&low_mem_buffer));

    bios_interrupt(0x13, &regs);

    ASSERT(!(regs.flags & EFL_CF), "Error reading from disk");

    memcpy(buffer, &low_mem_buffer, num_sectors * 512);
}
*/

int bdrive_read(blkdev_t* dev, uint64_t lba, size_t blocks, void* buffer)
{
    // TODO: do this in multiple reads?
    ASSERT(blocks * 512 <= 4096, "Read would have overflowed");

    struct bdrive_priv* priv = dev->priv;

    memset(&low_mem_disk_addr, 0, sizeof(low_mem_disk_addr));
    low_mem_disk_addr.size = 16;
    low_mem_disk_addr.num_sectors = blocks;
    low_mem_disk_addr.low_lba = lba & 0xffffffff;
    low_mem_disk_addr.high_lba = lba >> 32;
    low_mem_disk_addr.buffer = (uint32_t)&low_mem_buffer;

    struct int_regs regs;
    memset(&regs, 0, sizeof(regs));

    regs.eax = 0x4200;
    regs.esi = OFFOF((uint32_t)&low_mem_disk_addr);
    regs.ds = SEGOF((uint32_t)&low_mem_disk_addr);
    regs.edx = priv->drive_nr;

    // TODO: this should probably retry on failure?
    bios_interrupt(0x13, &regs);

    ASSERT(!(regs.flags & EFL_CF), "Error reading from disk");

    memcpy(buffer, &low_mem_buffer, blocks * dev->block_size);
    return 0;
}

int bdrive_write(blkdev_t* dev, uint64_t lba, size_t blocks, const void* buffer)
{
    // TODO: do this in multiple writes?
    ASSERT(blocks * 512 <= 4096, "Write would have overflowed");

    struct bdrive_priv* priv = dev->priv;

    memset(&low_mem_disk_addr, 0, sizeof(low_mem_disk_addr));
    low_mem_disk_addr.size = 16;
    low_mem_disk_addr.num_sectors = blocks;
    low_mem_disk_addr.low_lba = lba & 0xffffffff;
    low_mem_disk_addr.high_lba = lba >> 32;
    low_mem_disk_addr.buffer = (uint32_t)&low_mem_buffer;

    memcpy(&low_mem_buffer, buffer, blocks * dev->block_size);

    struct int_regs regs;
    memset(&regs, 0, sizeof(regs));

    regs.eax = 0x4300;
    regs.esi = OFFOF((uint32_t)&low_mem_disk_addr);
    regs.ds = SEGOF((uint32_t)&low_mem_disk_addr);
    regs.edx = priv->drive_nr;

    // TODO: this should probably retry on failure?
    bios_interrupt(0x13, &regs);

    ASSERT(!(regs.flags & EFL_CF), "Error writing to disk");

    return 0;
}

static void bdrive_destroy(struct device* dev)
{
    blkdev_t* blkdev = dev->internal_dev;

    kfree(blkdev->priv);
    kfree(blkdev);
    kfree(dev);
}

static struct device* bdrive_create(uint8_t drive_nr)
{
    struct device* dev = kallocz(sizeof(*dev));
    dev->type = DEVICE_TYPE_BLOCK;
    dev->destroy = bdrive_destroy;

    struct bdrive_priv* priv = kalloc(sizeof(*priv));
    priv->drive_nr = drive_nr;

    blkdev_t* blkdev = kalloc(sizeof(*blkdev));
    blkdev->write = bdrive_write;
    blkdev->read = bdrive_read;
    // likely not /actually/ the size of the sectors on a hard disk, but as
    // we're accessing through BIOS, this should be emulated
    blkdev->block_size = 512;
    blkdev->priv = priv;

    dev->internal_dev = blkdev;
    sprintf(dev->name, "hd%d", device_get_first_available_suffix("hd"));

    return dev;
}

static void bdrive_probe(struct driver* driver)
{
    if (!driver->first_probe)
        return;

    struct int_regs regs;

    for (int i = 0; i < 4; i++) {
        memset(&regs, 0, sizeof(regs));

        regs.eax = 0x0800; // read drive parameters
        regs.edx = 0x80 | i;
        // es:di = 0000:0000 for buggy BIOS'
        regs.es = 0;
        regs.edi = 0;

        bios_interrupt(0x13, &regs);
        // give up on on this drive if the call errored out
        if (regs.flags & EFL_CF)
            continue;

        debugf("[drive %02x] nr_hdds=%d", 0x80 | i, regs.edx & 0xff);
        if (regs.edx & 0xff > 0) {
            device_register(bdrive_create(0x80 | i));
        }
    }
}

struct driver bdrive_driver = {
    .name = "BIOS interrupt hard disk",
    .probe = bdrive_probe,
    .type_for = DEVICE_TYPE_BLOCK,
    .driver_priv = NULL,
};

static void bdrive_register_driver()
{
    driver_register(&bdrive_driver);
}
EXPORT_INIT(bdrive_register_driver);

