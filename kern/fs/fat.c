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

    blkdev_t* blkdev;
};

struct fat_file {
    // the cluster that this file starts on
    uint32_t start_cluster;
    // the current cluster that is being read
    uint32_t current_cluster;
    // the current offset into the file (absolute, in bytes)
    uint32_t current_offset;
    // the size of the file
    uint32_t size;

    // if the file is a directory, this will contain the listing
    // of that directory.
    char* dir_contents;
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

int read_cluster(fsdev_t* dev, uint32_t cluster, void* dst)
{
    struct fat_priv* priv = dev->priv;
    blkdev_t* blkdev = priv->blkdev;
    uint32_t sector = priv->start_lba + sector_of_cluster(priv, cluster);
    uint32_t sectors = priv->mbr.bpb.sectors_per_cluster;

    return blkdev->read(blkdev, sector, sectors, dst);
}

// given a current cluster number, determine the next cluster in the cluster chain.
uint32_t next_cluster(fsdev_t* dev, uint32_t current_cluster)
{
    struct fat_priv* priv = dev->priv;
    blkdev_t* blkdev = priv->blkdev;

    uint8_t fat_table[priv->bytes_per_sector];
    uint32_t fat_offset = current_cluster * 2;
    uint32_t fat_sector = priv->fat_start_sector + (fat_offset / priv->bytes_per_sector);
    uint32_t ent_offset = fat_offset % priv->bytes_per_sector;

    blkdev->read(blkdev, priv->start_lba + fat_sector, 1, fat_table);

    uint16_t next_cluster = *(uint16_t*)&fat_table[ent_offset];
    return next_cluster;
}

void* read_cluster_chain(fsdev_t* dev, uint32_t start_cluster, size_t* size)
{
    struct fat_priv* priv = dev->priv;
    size_t asize = 0;
    size_t offset = 0;
    void* data = kalloc(asize);

    uint32_t cluster = start_cluster;
    while (cluster > 0 && cluster <= FAT_CLUSTER_END) {
        asize += priv->bytes_per_cluster;
        void* ndata = krealloc(data, asize);
        if (!ndata) {
            kfree(data);
            return NULL;
        }
        data = ndata;

        read_cluster(dev, cluster, data + offset);
        offset += priv->bytes_per_cluster;
        cluster = next_cluster(dev, cluster);
    }

    if (size)
        *size = asize;
    return data;
}

// Find an entry within a directory. Will parse 8.3 names, as well as 11 char
// directory names. Flag is set depending on the outcome of the search.
//
// If the directory is not found, NULL is returned. Otherwise, a reference
// to the directory found within the buffer is returned.
struct fat_dir* find_dir_ent(const char* name, uint8_t* buf, size_t bufsz)
{
    ASSERT(sizeof(struct fat_dir) == 32, "bad FAT dir size");

    struct fat_dir* dir = (struct fat_dir*)buf;
    while (1) {
        if (dir->dir_name[0] == 0)
            break;

        // copy either the full dir name, or the name and extension to
        // a temp buffer, as they would be found in a file.
        char namebuf[12];
        memset(namebuf, ' ', 11);
        namebuf[11] = '\0';

        char* strdotpos = strchr(name, '.');
        if (strdotpos != NULL) {
            char* ext = strdotpos + 1;
            memcpy(namebuf, name, strdotpos - name);
            memcpy(namebuf + 8, ext, strlen(ext));
        } else {
            memcpy(namebuf, name, strlen(name));
        }

        // get the actual name into a null-terminated buffer so we can use
        // stricmp to compare case-inensitive
        char actual_namebuf[12];
        memcpy(actual_namebuf, dir->dir_name, 11);
        actual_namebuf[11] = '\0';

        if (stricmp(namebuf, actual_namebuf) == 0) {
            return dir;
        }

        // move onto the next entry, and if required get the next cluster
        // and follow it (or quit if it doesn't exist)
        dir++;
        if ((void*)dir > (void*)(buf + bufsz)) {
            return NULL;
        }
    }

    return NULL;
}

struct fat_dir* find_dir(fsdev_t* dev, const char** path, size_t pathlen)
{
    struct fat_priv* priv = dev->priv;
    blkdev_t* blkdev = priv->blkdev;

    // read the root directory in
    size_t rootdir_size = priv->bytes_per_sector * priv->nr_root_dir_sectors;
    uint8_t* cldata = kallocz(rootdir_size);
    size_t clsize = rootdir_size;

    blkdev->read(
        blkdev,
        priv->start_lba + priv->root_dir_sector,
        priv->nr_root_dir_sectors,
        cldata
    );

    // special case for the root directory
    if (pathlen == 1 && path[0][0]== '\0') {
        struct fat_dir* ret = kalloc(sizeof(*ret));
        memcpy(ret, cldata, sizeof(*ret));
        kfree(cldata);
        return ret;
    }

    struct fat_dir* dir;
    for (int i = 0; i < pathlen; i++) {
        dir = find_dir_ent(path[i], cldata, clsize);

        if (dir) {
            if (i + 1 != pathlen) {
                kfree(cldata);
                read_cluster_chain(dev, dir->cluster_low, &clsize);
            }
        } else {
            kfree(cldata);
            return NULL;
        }
    }

    // copy just this single directory entry
    struct fat_dir* ret = kalloc(sizeof(*ret));
    memcpy(ret, dir, sizeof(*ret));
    kfree(cldata);
    return ret;
}

char* get_dir_contents(fsdev_t* dev, struct fat_dir* pdir, int is_root)
{
    struct fat_priv* priv = dev->priv;
    blkdev_t* blkdev = priv->blkdev;
    uint8_t* cldata;
    size_t clsize;

    // root is a special case because there's no dirent as such which points
    // to it, so we just have to manually load it in
    if (is_root) {
        size_t rootdir_size = priv->bytes_per_sector * priv->nr_root_dir_sectors;
        cldata = kallocz(rootdir_size);
        clsize = rootdir_size;

        blkdev->read(
            blkdev,
            priv->start_lba + priv->root_dir_sector,
            priv->nr_root_dir_sectors,
            cldata
        );
    } else {
        cldata = read_cluster_chain(dev, pdir->cluster_low, &clsize);
    }

    struct fat_dir* dir = (struct fat_dir*)cldata;

    const size_t recsize = 20;
    size_t bufsize = 0;
    size_t bufoffset = 0;
    char* resbuf = kalloc(recsize);

    while ((void*)dir < (void*)(cldata + clsize) && dir->dir_name[0] != 0) {
        char namebuf[recsize];
        memset(namebuf, ' ', recsize - 1);
        namebuf[recsize - 1] = '\n';

        if (dir->attrs & FAT_ATTR_DIR) {
            memcpy(namebuf, dir->dir_name, 11);
            memcpy(namebuf + 13, "<DIR>", 5);
        } else if (dir->attrs & FAT_ATTR_VOLID) {
            memcpy(namebuf, dir->dir_name, 11);
            memcpy(namebuf + 13, "<VOL>", 5);
        } else {
            memcpy(namebuf, dir->file_name, 8);
            memcpy(namebuf + 9, dir->file_ext, 3);
        }

        bufsize += recsize;
        char* nresbuf = krealloc(resbuf, bufsize);
        if (!nresbuf) {
            kfree(resbuf);
            return NULL;
        }
        resbuf = nresbuf;

        memcpy(resbuf + bufoffset, namebuf, recsize);
        bufoffset += recsize;
        dir++;
    }

    char* nresbuf = krealloc(resbuf, bufsize + 1);
    if (!nresbuf) {
        kfree(resbuf);
        return NULL;
    }
    resbuf = nresbuf;
    resbuf = krealloc(resbuf, bufsize + 1);
    resbuf[bufsize] = '\0';
    kfree(cldata);

    return resbuf;
}

file_t* fat_open(fsdev_t* dev, const char** path, size_t pathlen)
{
    struct fat_priv* priv = dev->priv;

    struct fat_dir* dir = find_dir(dev, path, pathlen);

    if (!dir)
        return NULL;

    struct fat_file* file = kalloc(sizeof(*file));

    if (!(dir->attrs & (FAT_ATTR_DIR | FAT_ATTR_VOLID))) {
        file->start_cluster = dir->cluster_low;
        file->current_cluster = dir->cluster_low;
        file->current_offset = 0;
        file->size = dir->size;
        file->dir_contents = NULL;
    } else {
        // just in case any cluster ops are tried
        file->start_cluster = FAT_CLUSTER_END;
        file->current_cluster = FAT_CLUSTER_END;
        char* contents = get_dir_contents(dev, dir, dir->attrs & FAT_ATTR_VOLID);
        if (!contents) {
            kfree(file);
            file = NULL;
        } else {
            file->current_offset = 0;
            file->dir_contents = contents;
            file->size = strlen(contents);
        }
    }

    kfree(dir);
    return (file_t*)file;
}

int fat_seek(fsdev_t* dev, file_t* file, int mode, int32_t offset)
{
    return 0;

    // XXX: NOT IMPLEMENTED YET
    struct fat_priv* priv = dev->priv;
    struct fat_file* ffile = (struct fat_file*)file;
    uint32_t clbytes = priv->bytes_per_cluster;
    uint32_t reloffset;

    switch (mode) {
    default:
    case FSEEK_BEGIN:
        reloffset = 0;
        break;
    case FSEEK_CURRENT:
        reloffset = ffile->current_offset;
        break;
    }

    // offset within the current cluster
    uint32_t cloffset = reloffset - (ffile->current_cluster * clbytes);
    uint32_t newcloffset = cloffset + offset;

    // if the offset is within the current cluster, there's no work we
    // need to do apart from just adjusting the offset
    if (newcloffset >= 0 && newcloffset <= clbytes && cloffset < ffile->size) {
        ffile->current_offset += offset;
        return 0;
    }

    // just follow the whole cluster chain from the start.
    //
    // it would definitely be more efficient here to somehow figure out if the
    // new offset was past the current one, and just follow the cluster chain
    // from that point. however, this way is easier.
    uint32_t clus = ffile->start_cluster;
    uint32_t maxclus = (reloffset + offset) / clbytes;
    for (uint32_t i = 0; i < maxclus; i++) {
        clus = next_cluster(dev, clus);
        if (clus >= FAT_CLUSTER_END) {
            return -1;
        }
    }

    ffile->current_cluster = clus;
    ffile->current_offset = reloffset + offset;
    return 0;
}

int read_contents(file_t* file, size_t size, void* buf)
{
    struct fat_file* ffile = (struct fat_file*)file;

    if (ffile->current_offset >= ffile->size)
        return 0;

    size_t to_read = MIN(ffile->size - ffile->current_offset, size);
    memcpy(buf, ffile->dir_contents + ffile->current_offset, to_read);
    ffile->current_offset += to_read;
    return to_read;
}

int fat_read(fsdev_t* dev, file_t* file, size_t size, void* buf)
{
    struct fat_priv* priv = dev->priv;
    struct fat_file* ffile = (struct fat_file*)file;
    const uint32_t clbytes = priv->bytes_per_cluster;
    uint32_t clus = ffile->current_cluster;
    uint8_t clbuf[priv->bytes_per_cluster];
    const size_t full_size = MIN(size, ffile->size - ffile->current_offset);
    size_t remaining = full_size;

    if (full_size <= 0) {
        return 0;
    }

    if (ffile->dir_contents) {
        return read_contents(file, size, buf);
    }

    // read the first cluster (may not be full)
    if (read_cluster(dev, clus, clbuf) != priv->mbr.bpb.sectors_per_cluster) {
        return -1;
    }

    uint32_t clus_off = ffile->current_offset % clbytes;
    size_t firstclus_cnt = MIN(clbytes - clus_off, remaining);
    memcpy(buf, clbuf + clus_off, firstclus_cnt);
    buf += firstclus_cnt;
    remaining -= firstclus_cnt;

    // only move to the next cluster if this read moves out of it
    if (firstclus_cnt == clbytes - clus_off) {
        clus = next_cluster(dev, clus);
    }

    // read the rest of the full clusters
    while (remaining > clbytes) {
        if (clus >= FAT_CLUSTER_END) {
            ffile->current_cluster = clus;
            ffile->current_offset += size - remaining;
            return size - remaining;
        }

        read_cluster(dev, clus, clbuf);
        memcpy(buf, clbuf, clbytes);
        buf += clbytes;
        remaining -= clbytes;
        clus = next_cluster(dev, clus);
        debugf("remaining: %d, clbytes: %d", remaining, clbytes);
    }

    // read the final partial cluster (if there is one)
    if (remaining > 0) {
        read_cluster(dev, clus, clbuf);
        size_t lastclus_cnt = MIN(clbytes, remaining);
        memcpy(buf, clbuf, lastclus_cnt);
        remaining -= lastclus_cnt;
    }

    ffile->current_cluster = clus;
    ffile->current_offset += full_size - remaining;

    return full_size - remaining;
}

void fat_close(fsdev_t* dev, file_t* file)
{
    struct fat_file* ffile = (struct fat_file*)file;
    if (ffile->dir_contents)
        kfree(ffile->dir_contents);

    kfree(file);
}

static struct device* fat_create(struct device* invoker, blkdev_t* blkdev, uint32_t start_lba, uint32_t num_sectors)
{
    struct device* dev = kallocz(sizeof(*dev));
    struct fat_priv* priv = kalloc(sizeof(*priv));
    fsdev_t* fsdev = kallocz(sizeof(*fsdev));

    fsdev->open = fat_open;
    fsdev->close = fat_close;
    fsdev->read = fat_read;
    dev->device_priv = priv;
    fsdev->priv = priv;

    dev->internal_dev = fsdev;
    dev->type = DEVICE_TYPE_FS;

    // the name of a partition is the blkdev suffixed with a partition index
    char prefix[64];
    sprintf(prefix, "%sp", invoker->name);
    sprintf(dev->name, "%s%d", prefix, device_get_first_available_suffix(prefix));

    priv->start_lba = start_lba;
    priv->num_sectors = num_sectors;
    priv->blkdev = blkdev;

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

