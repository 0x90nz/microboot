#pragma once

#include <stdint.h>

struct fs* fs_init(uint8_t drive_num);

enum fs_type {
    FS_INVALID,
    FS_EXT2
};

typedef struct fs_dir {
    const char* name;
    void* _internal_dir;
} fs_dir_t;

typedef struct fs {
    enum fs_type type;
    fs_dir_t* root_dir;
    void* _internal_fs;
} fs_t;

void fs_list_dir(fs_t* fs, fs_dir_t* dir);
fs_dir_t* fs_traverse(fs_t* fs, const char* path);

#define FS_PATH_SEPARATOR       "/"
#define FS_PATH_SEPARATOR_CHAR  '/'