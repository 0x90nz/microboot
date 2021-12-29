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

int bdrive_read4k(blkdev_t* dev, uint64_t lba, size_t blocks, void* buffer)
{
    ASSERT(blocks * dev->block_size <= 4096, "Read would have overflowed");

    struct bdrive_priv* priv = dev->priv;

    memset(&low_mem_disk_addr, 0, sizeof(struct disk_addr));
    low_mem_disk_addr.size = 16;
    low_mem_disk_addr.num_sectors = blocks;
    low_mem_disk_addr.low_lba = lba & 0xffffffff;
    low_mem_disk_addr.high_lba = lba >> 32;
    uint16_t buf_seg = SEGOF((uint32_t)&low_mem_buffer);
    uint16_t buf_off = OFFOF((uint32_t)&low_mem_buffer);
    low_mem_disk_addr.buffer = buf_off | buf_seg << 16;

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

int bdrive_read(blkdev_t* dev, uint64_t lba, size_t blocks, void* buffer)
{
    int32_t blocks_left = blocks;
    uint32_t max_blocks = 4096 / dev->block_size;
    void* cursor = buffer;
    uint32_t offset = 0;

    while (blocks_left > 0) {
        uint32_t nr_blocks = MIN(blocks_left, max_blocks);
        if (bdrive_read4k(dev, lba + offset, nr_blocks, cursor)) {
            return -1;
        }

        offset += nr_blocks;
        cursor += nr_blocks * dev->block_size;
        blocks_left -= nr_blocks;
    }

    return blocks;
}

int bdrive_write4k(blkdev_t* dev, uint64_t lba, size_t blocks, const void* buffer)
{
    // TODO: do this in multiple writes?
    ASSERT(blocks * 512 <= dev->block_size, "Write would have overflowed");

    struct bdrive_priv* priv = dev->priv;

    memset(&low_mem_disk_addr, 0, sizeof(low_mem_disk_addr));
    low_mem_disk_addr.size = 16;
    low_mem_disk_addr.num_sectors = blocks;
    low_mem_disk_addr.low_lba = lba & 0xffffffff;
    low_mem_disk_addr.high_lba = lba >> 32;
    uint16_t buf_seg = SEGOF((uint32_t)&low_mem_buffer);
    uint16_t buf_off = OFFOF((uint32_t)&low_mem_buffer);
    low_mem_disk_addr.buffer = buf_off | buf_seg << 16;

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

// there is a bit of repetition here with bdrive_write and bdrive_read.
// perhaps this could be factored out in the future.
int bdrive_write(blkdev_t* dev, uint64_t lba, size_t blocks, const void* buffer)
{
    uint32_t blocks_left = blocks;
    uint32_t max_blocks = 4096 / dev->block_size;
    const void* cursor = buffer;
    uint32_t offset = 0;

    while (blocks_left > 0) {
        uint32_t nr_blocks = MIN(blocks_left, max_blocks);
        if (bdrive_write4k(dev, lba + offset, nr_blocks, cursor)) {
            return -1;
        }

        offset += nr_blocks;
        cursor += nr_blocks * dev->block_size;
        blocks_left -= nr_blocks;
    }

    return blocks;
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

static void bdrive_reset(uint8_t drive_nr)
{
    struct int_regs regs;
    memset(&regs, 0, sizeof(regs));

    regs.eax = 0x0000;
    regs.edx = drive_nr;

    bios_interrupt(0x13, &regs);
}

static void bdrive_probe(struct driver* driver)
{
    if (!driver->first_probe)
        return;

    struct int_regs regs;

    for (int i = 0; i < 4; i++) {
        uint8_t drive_nr = 0x80 | i;

        memset(&regs, 0, sizeof(regs));

        regs.eax = 0x0800; // read drive parameters
        regs.edx = drive_nr;
        regs.es = 0;
        regs.edi = 0;

        bios_interrupt(0x13, &regs);
        // give up on on this drive if the call errored out
        if (regs.flags & EFL_CF) {
            bdrive_reset(drive_nr);
            continue;
        }

        debugf("[drive %02x] nr_hdds=%d", drive_nr, regs.edx & 0xff);
        if (regs.edx & 0xff > 0) {
            struct device* dev = bdrive_create(drive_nr);
            device_register(dev);
        }
    }

    // do one last reset just in case something weird happened detecting drives
    bdrive_reset(0x80);
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

