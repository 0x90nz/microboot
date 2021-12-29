#include "fat.h"
#include <stdint.h>
#include <export.h>
#include "mbr.h"
#include "../io/driver.h"
#include "../stdlib.h"
#include "../alloc.h"

struct fat_bpb {
    uint8_t reserved0[3]; // boot jmp
    uint8_t oem_ident[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t nr_reserved_sectors;
    uint8_t nr_fats;
    uint16_t nr_root_entries;
    uint16_t nr_total_sectors;
    uint8_t media_descriptor;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t nr_heads;
    uint32_t nr_hidden_sectors;
    uint32_t nr_large_sectors;
} __attribute__((packed));

struct fat_ebr {
    uint8_t drive_nr;
    uint8_t reserved0;
    uint8_t signature;
    uint8_t volume_id[4];
    uint8_t volume_label[11];
    uint8_t system_ident[8];
    uint8_t boot_code[448];
    uint16_t boot_signature;
} __attribute__((packed));

struct fat_mbr {
    struct fat_bpb bpb;
    struct fat_ebr ebr;
} __attribute__((packed));

struct fat_dir {
    // 8.3 format name and extension
    union {
        struct {
            uint8_t file_name[8];
            uint8_t file_ext[3];
        };
        uint8_t dir_name[11];
        uint8_t vol_name[11];
    };
    // File attributes (see enum fat_attrs)
    uint8_t attrs;
    uint8_t reserved0;
    uint8_t created_tenths;
    // created time in h(5):m(6):s(5)*2 format
    uint16_t created_time;
    // created date in y(7):m(4):d(5) format
    uint16_t created_date;
    // last access in same format as created_date
    uint16_t last_access_date;
    // technically always zero on fat16
    uint16_t cluster_high;
    // last modification time in same format as created_time
    uint16_t last_mod_time;
    // last modification date in the same format as created_date
    uint16_t last_mod_date;
    // the cluster that this entry points to
    uint16_t cluster_low;
    // the size of the file (in bytes)
    uint32_t size;
} __attribute__((packed));

enum fat_attrs {
    // Read-only
    FAT_ATTR_RDONLY     = (1 << 0),
    FAT_ATTR_HIDDEN     = (1 << 1),
    FAT_ATTR_SYSTEM     = (1 << 2),
    // Volume ID
    FAT_ATTR_VOLID      = (1 << 3),
    // Directory
    FAT_ATTR_DIR        = (1 << 4),
    FAT_ATTR_ARCHIVE    = (1 << 5),
};

struct fat_priv {
    uint32_t start_lba;
    uint32_t num_sectors;
    uint32_t bytes_per_cluster;
    uint32_t bytes_per_sector;

    uint32_t data_start_sector;
    uint32_t fat_start_sector;
    uint32_t root_dir_sector;
    uint32_t nr_root_dir_sectors;

    struct fat_mbr mbr;
    uint8_t fat_table[512];
};

#define FAT_CLUSTER_END     0xfff8

uint32_t sector_of_cluster(struct fat_priv* priv, uint32_t cluster)
{
    return ((cluster - 2) * priv->mbr.bpb.sectors_per_cluster) + priv->data_start_sector;
}

// the inverse (really just a rearrangement) of the above, instead of converting
// a sector to a cluster, convert a cluster number to the starting sector
uint32_t cluster_of_sector(struct fat_priv* priv, uint32_t sector)
{
    return ((sector - priv->data_start_sector) / priv->mbr.bpb.sectors_per_cluster) + 2;
}

int read_cluster(struct device* dev, uint32_t cluster, void* dst)
{
    struct fat_priv* priv = dev->device_priv;
    blkdev_t* blkdev = dev->internal_dev;

    return blkdev->read(
        blkdev,
        priv->start_lba + sector_of_cluster(priv, cluster),
        priv->mbr.bpb.sectors_per_cluster,
        dst
    );
}

// given a current cluster number, determine the next cluster in the cluster chain.
uint32_t next_cluster(struct device* dev, uint32_t current_cluster)
{
    struct fat_priv* priv = dev->device_priv;

    uint32_t offset = current_cluster * 2;
    uint32_t ent_offset = offset % priv->bytes_per_sector;
    uint16_t next_cluster = *(uint16_t*)&priv->fat_table[ent_offset];

    return next_cluster;
}

// read all the directory entries from a given buffer
//
// if the end of the buffer is reached, then a non-zero value is returned.
// if the end of the directory chain is reached, a zero value is returned
int read_directory_buffer(uint8_t* buf, size_t bufsz)
{
    ASSERT(sizeof(struct fat_dir) == 32, "bad FAT dir size");

    struct fat_dir* dir = (struct fat_dir*)buf;
    while (1) {
        if (dir->dir_name[0] == 0)
            break;

        if (dir->attrs & FAT_ATTR_DIR) {
            char namebuf[32];
            memset(namebuf, 0, sizeof(namebuf));
            memcpy(namebuf, dir->dir_name, 11);
            debugf("%s", namebuf);
        }

        // move onto the next entry, and if required get the next cluster
        // and follow it (or quit if it doesn't exist)
        dir++;
        if ((void*)dir > (buf + bufsz)) {
            return 1;
        }
    }
    return 0;
}

// simple debug read for root directory
void d_read_directory(struct device* dev)
{
    struct fat_priv* priv = dev->device_priv;

    size_t rootdir_size = priv->bytes_per_sector * priv->nr_root_dir_sectors;
    uint8_t rootdir[rootdir_size];
    blkdev_t* blkdev = dev->internal_dev;

    blkdev->read(
        blkdev,
        priv->start_lba + priv->root_dir_sector,
        priv->nr_root_dir_sectors,
        rootdir
    );
    debugf("rc=%d", read_directory_buffer(rootdir, rootdir_size));
}

static struct device* fat_create(struct device* invoker, blkdev_t* blkdev, uint32_t start_lba, uint32_t num_sectors)
{
    struct device* dev = kallocz(sizeof(*dev));

    // the name of a partition is the blkdev suffixed with a partition index
    char prefix[64];
    sprintf(prefix, "%sp", invoker->name);
    sprintf(dev->name, "%s%d", prefix, device_get_first_available_suffix(prefix));
    dev->internal_dev = blkdev;

    struct fat_priv* priv = kalloc(sizeof(*priv));
    dev->device_priv = priv;

    priv->start_lba = start_lba;
    priv->num_sectors = num_sectors;

    ASSERT(sizeof(struct fat_mbr) == 512, "bad FAT MBR size");
    blkdev->read(blkdev, start_lba, 1, &priv->mbr);

    uint32_t bytes_per_sector = priv->mbr.bpb.bytes_per_sector;
    // at the moment we only deal with 512 byte sectors
    ASSERT(bytes_per_sector == 512, "Bad FAT sector size");

    priv->bytes_per_sector = bytes_per_sector;
    priv->bytes_per_cluster = bytes_per_sector * priv->mbr.bpb.sectors_per_cluster;
    debugf(
        "%d bytes per sector, %d bytes per cluster",
        priv->bytes_per_sector,
        priv->bytes_per_cluster
    );

    uint32_t nr_root_dir_sectors = ((priv->mbr.bpb.nr_root_entries * 32) + (bytes_per_sector - 1))
        / bytes_per_sector;

    priv->fat_start_sector = priv->mbr.bpb.nr_reserved_sectors;
    priv->data_start_sector = priv->mbr.bpb.nr_reserved_sectors +
        (priv->mbr.bpb.nr_fats * priv->mbr.bpb.sectors_per_fat) + nr_root_dir_sectors;
    priv->root_dir_sector = priv->data_start_sector - nr_root_dir_sectors;
    priv->nr_root_dir_sectors = nr_root_dir_sectors;

    debugf(
        "fat start %d, data start %d, root dir %d",
        priv->fat_start_sector,
        priv->data_start_sector,
        priv->root_dir_sector
    );

    d_read_directory(dev);
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

