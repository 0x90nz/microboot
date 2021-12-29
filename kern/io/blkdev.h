#pragma once

#include <stdint.h>

typedef struct blockdev blkdev_t;

/*
 * These definitions lay out the interface that all block devices use.
 *
 * `lba` is the logical block address (in whatever units the block device uses
 * for addressing, the block size of which can be found in the `block_size`
 * member).
 *
 * `blocks` is the number of blocks to transfer. Each block is `block_size`
 * large, and will be placed (or read from) the given buffer
 *
 * The return value is either the number of blocks written (which must be
 * a positive number), or an error code, which must be negative.
 */
typedef int (*block_write_t)(blkdev_t* dev, uint64_t lba, size_t blocks, const void* buffer);
typedef int (*block_read_t)(blkdev_t* dev, uint64_t lba, size_t blocks, void* buffer);

struct blockdev {
    // Function pointer which will write data to the device
    block_write_t write;
    // Function pointer which will read data from the device
    block_read_t read;
    // The size of the blocks that the device can read (e.g. the sector size
    // for a hard disk)
    uint32_t block_size;
    void* priv;
};

