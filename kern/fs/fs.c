/**
 * @file fs.c
 * @brief provides a filesystem independent interface for disk operations
 */

#include <export.h>
#include "fs.h"
#include "mbr.h"
#include "../alloc.h"
#include "../io/bios_drive.h"

#include "../stdlib.h"
#include "../env.h"

struct fs_file_entry {
    char state;
    fs_file_priv_t priv;
    fs_t* fs;
};

static env_t* mounts;
static fs_t* rootfs;

#define OFTABLE_OPEN    (1 << 0)
#define OFTABLE_SIZE    64
static struct fs_file_entry oftable[OFTABLE_SIZE];

// Find the first open entry in the oftable. Return -1 if no open entry exists
static fs_file_t first_free_entry()
{
    for (int i = 0; i < OFTABLE_SIZE; i++) {
        if (!(oftable[i].state & OFTABLE_OPEN)) {
            return i;
        }
    }
    return FS_FILE_INVALID;
}

static const fs_file_priv_t fs_get_root(fs_t* fs)
{
    if (fs->ops->get_root)
        return fs->ops->get_root(fs);
    return NULL;
}

static fs_file_priv_t fs_getfile(fs_t* fs, fs_file_priv_t dir, const char* name)
{
    if (fs->ops->getfile)
        return fs->ops->getfile(fs, dir, name);
    return NULL;
}

const static fs_file_priv_t fs_traverse(fs_t* fs, const char* path)
{
    debugf("traverse # fs=%08x, path='%s'", fs, path);
    size_t len = strlen(path) * sizeof(char) + 1;
    char* buffer = kallocz(len + 2);
    memcpy(buffer, path, len);

    int count = 1;
    char* token = strtok(buffer, FS_PATH_SEPARATOR);
    while (strtok(NULL, FS_PATH_SEPARATOR) != NULL) { count++; }

    fs_file_priv_t root = fs_get_root(fs);
    // If the path is empty, we just return the root dir
    if (!token) {
        kfree(buffer);
        debugf("%08x", root);
        return root;
    }

    char** parts = kalloc(sizeof(char*) * count);

    char* tmp = token;
    for (int i = 0; i < count; i++) {
        parts[i] = tmp;
        tmp += strlen(tmp) + 1;
    }

    fs_file_priv_t current = root;
    for (int i = 0; i < count; i++) {
        fs_file_priv_t new_dir = fs_getfile(fs, current, parts[i]);

        if (current != root && fs->ops->destroy)
            fs->ops->destroy(fs, current);

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


/**
 * @brief Initialise any available FS drivers for each partition.
 * @param drive_num the drive number to search for the filesystem on
 */
void fs_init()
{
    mounts = env_init();

    // struct mbr_sector* bootsect = kalloc(512);
    // bdrive_read(drive_num, 1, 0, bootsect);
    // struct fs* fs = kalloc(sizeof(struct fs));
    // fs->type = FS_INVALID;
}

/**
 * @brief Open a file. Files may be either directly specified by a path
 * relative to the default driver, or with a descriptor. A descriptor can
 * be prepended to a path with a '$' symbol, e.g. "hd$test.txt"
 *
 * @param name the path to the file
 * @return fs_file_t file on success, FS_FILE_INVALID on failure
 */
fs_file_t fs_open(const char* name)
{
    fs_file_t ent = first_free_entry();
    if (ent == FS_FILE_INVALID)
        return ent;

    fs_t* fs = NULL;

    // find if we have a descriptor before the path.
    const char* path = strstr(name, "$");
    if (path) {
        char drive[64];
        memcpy(drive, name, path - name);
        drive[path - name] = '\0';
        path++;
        fs_t* mount = env_get(mounts, drive, fs_t*);
        if (mount) {
            fs = mount;
        } else {
            return FS_FILE_INVALID;
        }
    } else {
        path = name;
        fs = rootfs;
    }

    fs_file_priv_t priv = fs_traverse(fs, path);
    if (!priv)
        return FS_FILE_INVALID;

    oftable[ent].fs = fs;
    oftable[ent].state |= OFTABLE_OPEN;
    oftable[ent].priv = priv;
    return ent;
}
EXPORT_SYM(fs_open);

/**
 * @brief List the contents of the provided directory
 *
 * @param dir the directory to list
 */
void fs_flist(fs_file_t dir)
{
    struct fs_file_entry* ent = &oftable[dir];
    if (ent->fs->ops->ls)
        ent->fs->ops->ls(ent->fs, ent->priv);
}
EXPORT_SYM(fs_flist);

/**
 * @brief Get the size (in bytes) of a given file
 *
 * @param file the file
 * @return uint32_t the size of the file in bytes
 */
uint32_t fs_fsize(fs_file_t file)
{
    struct fs_file_entry* ent = &oftable[file];
    if (ent->fs->ops->fsize)
        return ent->fs->ops->fsize(ent->fs, ent->priv);
    return 0;
}
EXPORT_SYM(fs_fsize);

/**
 * @brief Read data from a given file
 *
 * @param file the file to read from
 * @param offset the offset (in bytes) from the start of the file at which
 * reading should begin
 * @param size the size (in bytes) to read
 * @param buffer the buffer into which the data should be read
 * @return uint32_t the number of bytes actually read
 */
uint32_t fs_fread(fs_file_t file, uint32_t offset, size_t size, void* buffer)
{
    struct fs_file_entry* ent = &oftable[file];
    if (ent->fs->ops->read)
        return ent->fs->ops->read(ent->fs, ent->priv, offset, size, buffer);
    return 0;
}
EXPORT_SYM(fs_fread);

/**
 * @brief Destroy a file reference
 *
 * @param file the file to destroy
 */
void fs_fdestroy(fs_file_t file)
{
    struct fs_file_entry* ent = &oftable[file];
    if (ent->fs->ops->destroy)
        ent->fs->ops->destroy(ent->fs, ent->priv);

    oftable[file].state &= ~OFTABLE_OPEN;
}
EXPORT_SYM(fs_fdestroy);

/**
 * @brief Mount a filesystem
 *
 * @param name the name of the filesystem
 * @param fs the filesystem
 */
void fs_mount(const char* name, fs_t* fs)
{
    env_put(mounts, name, fs);
}
EXPORT_SYM(fs_mount);
