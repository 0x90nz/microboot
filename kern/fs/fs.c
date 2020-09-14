/**
 * @file fs.c
 * @brief provides a filesystem independent interface for disk operations
 */

#include "fs.h"
#include "mbr.h"
#include "ext2.h"
#include "../alloc.h"
#include "../io/bios_drive.h"

#include "../stdlib.h"

/**
 * @brief Initialise a filesystem. Currently only supports one ext2 partition
 * 
 * @param drive_num the drive number to search for the filesystem on
 * @return struct fs* the filesystem which was found on that drive
 */
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

/**
 * @brief List the contents of the provided directory
 * 
 * @param fs the filesystem
 * @param dir the directory to list
 */
void fs_list_dir(fs_t* fs, fs_dir_t dir)
{
    if (fs->ops->ls)
        fs->ops->ls(fs, dir);
}

/**
 * @brief Get the root directory of a filesystem
 * 
 * @param fs the filesystem
 * @return const fs_dir_t the root directory **do not** attempt to free this pointer
 */
const fs_dir_t fs_get_root(fs_t* fs)
{
    if (fs->ops->get_root)
        return fs->ops->get_root(fs);
    return NULL;
}

/**
 * @brief Get the size (in bytes) of a given file
 * 
 * @param fs the filesystem
 * @param file the file
 * @return uint32_t the size of the file in bytes
 */
uint32_t fs_fsize(fs_t* fs, fs_file_t file)
{
    if (fs->ops->fsize)
        return fs->ops->fsize(fs, file);
    return 0;
}

/**
 * @brief Get a file which resides within the given directory
 * 
 * @param fs the filesystem
 * @param dir the directory in which the file is
 * @param name the name of the file
 * @return fs_file_t the file, this should be freed once no longer needed
 */
fs_file_t fs_getfile(fs_t* fs, fs_dir_t dir, const char* name)
{
    if (fs->ops->getfile)
        return fs->ops->getfile(fs, dir, name);
    return NULL;
}

/**
 * @brief Read data from a given file
 * 
 * @param fs the filesystem
 * @param file the file to read from
 * @param offset the offset (in bytes) from the start of the file at which
 * reading should begin
 * @param size the size (in bytes) to read
 * @param buffer the buffer into which the data should be read
 * @return uint32_t the number of bytes actually read
 */
uint32_t fs_read(fs_t* fs, fs_file_t file, uint32_t offset, size_t size, void* buffer)
{
    if (fs->ops->read)
        return fs->ops->read(fs, file, offset, size, buffer);
    return 0;
}

/**
 * @brief Destroy a file reference
 * 
 * @param fs the filesystem
 * @param file the file to destroy
 */
void fs_destroy(fs_t* fs, fs_file_t file)
{
    if (fs->ops->destroy)
        fs->ops->destroy(fs, file);
}

/**
 * @brief Traverse a path from the root directory to a directory
 * @warning This function is a work in progress. Do not expect it to work
 * 
 * @param fs the filesystem
 * @param path the path to traverse
 * @return const fs_dir_t the final directory of the traversed path
 */
const fs_dir_t fs_traverse(fs_t* fs, const char* path)
{
    fs_dir_t root = fs_get_root(fs);

    size_t len = strlen(path) * sizeof(char) + 1;
    char* buffer = kallocz(len + 2);
    memcpy(buffer, path, len);

    int count = 1;
    char* token = strtok(buffer, FS_PATH_SEPARATOR);
    while (strtok(NULL, FS_PATH_SEPARATOR) != NULL) { count++; }

    // If the path is empty, we just return the root dir
    if (!token)
        return root;

    char** parts = kalloc(sizeof(char*) * count);

    char* tmp = token;
    for (int i = 0; i < count; i++) {
        parts[i] = tmp;
        tmp += strlen(tmp) + 1;
    }

    fs_dir_t current = root;
    for (int i = 0; i < count; i++) {
        fs_file_t new_dir = fs_getfile(fs, current, parts[i]);

        if (current != root)
            kfree(current);
        
        if (new_dir) {
            current = new_dir;
        } else {
            current = NULL;
            break;
        }
    }

    kfree(parts);
    kfree(buffer);

    return current;
}