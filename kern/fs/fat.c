#include "fat.h"
#include <stdint.h>
#include <export.h>
#include "mbr.h"
#include "../io/driver.h"
#include "../stdlib.h"
#include "../alloc.h"

struct fat_priv {
    uint64_t start_lba;
    uint64_t num_sectors;
};

static struct device* fat_create(struct device* invoker, blkdev_t* blkdev, uint32_t start_lba, uint32_t num_sectors)
{
   struct device* dev = kallocz(sizeof(*dev));

   // the name of a partition is the blkdev suffixed with a partition index
   char prefix[64];
   sprintf(prefix, "%sp", invoker->name);
   sprintf(dev->name, "%s%d", prefix, device_get_first_available_suffix(prefix));

   struct fat_priv* priv = kalloc(sizeof(*priv));

   return dev;
}

static void fat_probe_directed(struct driver* driver, struct device* invoker)
{
    if (!invoker || invoker->type != DEVICE_TYPE_BLOCK)
        return;

    blkdev_t* blkdev = device_get_blkdev(invoker);

    struct mbr_sector mbr;
    blkdev->read(blkdev, 0, 1, &mbr);

    for (int i = 0; i < 4; i++) {
        struct mbr_part_entry* part = &mbr.partitions[i];
        if (part->type == 0x0e) {
            device_register(fat_create(invoker, blkdev, part->start_lba, part->num_sectors));
        }
    }
}

struct driver fat_driver = {
    .name = "FAT filesystem",
    .probe_directed = fat_probe_directed,
    .type_for = DEVICE_TYPE_FS,
    .depends_on = DEVICE_TYPE_BLOCK,
    .driver_priv = NULL,
};

static void fat_register_driver()
{
    driver_register(&fat_driver);
}
EXPORT_INIT(fat_register_driver);

