#pragma once

#include <stdint.h>
#include <stddef.h>

enum fs_type {
    FS_INVALID,
    FS_EXT2,
    FS_ENVFS
};

enum fs_ftype {
    FS_DIR,
    FS_FILE,
    FS_CHAR
};

#define FS_FILE_INVALID     -1

typedef void* fs_priv_t;
typedef void* fs_file_priv_t;
typedef int fs_file_t;

typedef struct fs_ops fs_ops_t;

typedef struct fs {
    enum fs_type type;
    fs_priv_t fs_priv;
    const fs_ops_t* ops;
} fs_t;

typedef struct fs_ops {
    uint32_t (*read)(fs_t*, fs_file_priv_t, uint32_t, size_t, void*);
    uint32_t (*fsize)(fs_t*, fs_file_priv_t);
    void (*ls)(fs_t*, fs_file_priv_t);
    fs_file_priv_t (*getfile)(fs_t*, fs_file_priv_t, const char*);
    const fs_file_priv_t (*get_root)(fs_t*);
    void (*destroy)(fs_t*, fs_file_priv_t);
} fs_ops_t;

void fs_init();
fs_file_t fs_open(const char* name);
void fs_flist(fs_file_t file);
uint32_t fs_fsize(fs_file_t file);
uint32_t fs_fread(fs_file_t file, uint32_t offset, size_t size, void* buffer);
void fs_fdestroy(fs_file_t file);
void fs_mount(const char* name, fs_t* fs);

#define FS_PATH_SEPARATOR       "/"
#define FS_PATH_SEPARATOR_CHAR  '/'
