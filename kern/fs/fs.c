#include "fs.h"
#include "mbr.h"
#include "ext2.h"
#include "../alloc.h"
#include "../io/bios_drive.h"

#include "../stdlib.h"

struct fs* fs_init(uint8_t drive_num)
{
    struct mbr_sector* bootsect = kalloc(512);
    bdrive_read(drive_num, 1, 0, bootsect);
    struct fs* fs = kalloc(sizeof(struct fs));

    fs->type = FS_INVALID;

    for (int i = 0; i < 4; i++) {
        if (bootsect->partitions[i].drive_attributes == 0x83) {
#ifdef EXT2_ENABLE
            struct ext2_fs* efs = ext2_init(
                drive_num, 
                bootsect->partitions[i].start_lba, 
                bootsect->partitions[i].num_sectors
            );

            fs->type = FS_EXT2;
            fs->root_dir = kalloc(sizeof(struct fs_dir));
            fs->root_dir->_internal_dir = efs->root_inode;
            fs->_internal_fs = efs;
#endif
        }
    }

    kfree(bootsect);

    return fs;
}

void fs_list_dir(fs_t* fs, fs_dir_t* dir)
{
    switch (fs->type) {
    case FS_EXT2:
        ext2_listdir(
            (struct ext2_fs*)fs->_internal_fs, 
            (struct ext2_inode*)dir->_internal_dir
        );
        break;

    case FS_INVALID:
    default:
        break;
    }
}

void fs_get_root(fs_t* fs, fs_dir_t* dir)
{
    switch (fs->type) {
    case FS_EXT2:
    {
        struct ext2_fs* efs = (struct ext2_fs*)fs->_internal_fs;
        dir->_internal_dir = efs->root_inode;
        break;
    }

    default:
    case FS_INVALID:
        break;
    }
}