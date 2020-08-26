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
            fs->fs_priv = efs;
            fs->ops = ext2_get_ops();
#endif
        }
    }

    kfree(bootsect);

    return fs;
}

void fs_list_dir(fs_t* fs, fs_dir_t dir)
{
    fs->ops->ls(fs, dir);
}

const fs_dir_t fs_get_root(fs_t* fs)
{
    return fs->ops->get_root(fs);
}

uint32_t fs_fsize(fs_t* fs, fs_file_t file)
{
    return fs->ops->fsize(fs, file);
}

fs_file_t fs_getfile(fs_t* fs, fs_dir_t dir, const char* name)
{
    return fs->ops->getfile(fs, dir, name);
}

uint32_t fs_read(fs_t* fs, fs_file_t file, uint32_t offset, size_t size, void* buffer)
{
    return fs->ops->read(fs, file, offset, size, buffer);
}

const fs_dir_t fs_traverse(fs_t* fs, const char* path)
{
    size_t len = strlen(path) * sizeof(char) + 1;
    char* buffer = kallocz(len + 2);
    memcpy(buffer, path, len);

    int count = 1;
    char* token = strtok(buffer, FS_PATH_SEPARATOR);
    while (strtok(NULL, FS_PATH_SEPARATOR) != NULL) { count++; }

    char** parts = kalloc(sizeof(char*) * count);

    char* tmp = token;
    for (int i = 0; i < count; i++) {
        parts[i] = tmp;
        tmp += strlen(tmp) + 1;
    }

    fs_dir_t root = fs_get_root(fs);

    for (int i = 0; i < count; i++) {
        
    }

    kfree(parts);
    kfree(buffer);

    return NULL;
}