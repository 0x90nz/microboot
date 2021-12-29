#include "fs.h"
#include "../io/driver.h"
#include "../io/fsdev.h"
#include "../config.h"
#include "../stdlib.h"
#include "../alloc.h"

struct filehandle {
    file_t* file;
    fsdev_t* fs;
};

filehandle_t* fs_open(const char* path)
{
    const char* def_fs = config_getstrns("sys", "def_fs");
    struct device* dev = device_get_by_name(def_fs);
    fsdev_t* fs = device_get_fs(dev);

    char* pathbuf = strdup(path);

    int pathlen = 1;
    char* token = strtok(pathbuf, FS_PATH_SEPARATOR);
    while (strtok(NULL, " ") != NULL) { pathlen++; }

    char** parts = kalloc(sizeof(char*) * pathlen);

    char* tmp = token;
    for (int i = 0; i < pathlen; i++) {
        parts[i] = tmp;
        tmp += strlen(tmp) + 1;
    }

    file_t* file = fs->open(fs, (const char**)parts, pathlen);
    filehandle_t* handle = NULL;
    if (file) {
        handle = kalloc(sizeof(*handle));
        handle->file = file;
        handle->fs = fs;
    }

    kfree(pathbuf);
    kfree(parts);

    return handle;
}

int fs_read(filehandle_t* handle, void* buf, size_t count)
{
    if (!handle)
        return 0;

    if (!handle->fs->read)
        return 0;

    return handle->fs->read(handle->fs, handle->file, count, buf);
}

void fs_close(filehandle_t* handle)
{
    if (!handle)
        return;

    if (handle->fs->close)
        handle->fs->close(handle->fs, handle->file);
    kfree(handle);
}

