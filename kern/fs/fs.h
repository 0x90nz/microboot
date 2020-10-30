#pragma once

#include <stdint.h>
#include <stddef.h>

struct fs* fs_init(uint8_t drive_num);

enum fs_type {
    FS_INVALID,
    FS_EXT2,
    FS_KFS
};

typedef void* fs_priv_t;
typedef void* fs_file_t;

typedef struct fs_ops fs_ops_t;

typedef struct fs {
    enum fs_type type;
    fs_priv_t fs_priv;
    const fs_ops_t* ops;
} fs_t;

typedef struct fs_ops {
    uint32_t (*read)(fs_t*, fs_file_t, uint32_t, size_t, void*);
    uint32_t (*fsize)(fs_t*, fs_file_t);
    void (*ls)(fs_t*, fs_file_t);
    fs_file_t (*getfile)(fs_t*, fs_file_t, const char*);
    const fs_file_t (*get_root)(fs_t*);
    void (*destroy)(fs_t*, fs_file_t);
} fs_ops_t;

void fs_list_dir(fs_t* fs, fs_file_t dir);
const fs_file_t fs_get_root(fs_t* fs);
const fs_file_t fs_traverse(fs_t* fs, const char* path);
uint32_t fs_fsize(fs_t* fs, fs_file_t file);
fs_file_t fs_getfile(fs_t* fs, fs_file_t dir, const char* name);
uint32_t fs_read(fs_t* fs, fs_file_t file, uint32_t offset, size_t size, void* buffer);
void fs_destroy(fs_t* fs, fs_file_t file);

#define FS_PATH_SEPARATOR       "/"
#define FS_PATH_SEPARATOR_CHAR  '/'