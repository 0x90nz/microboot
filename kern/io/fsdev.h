#pragma once

#include <stddef.h>

typedef struct fsdev fsdev_t;
typedef struct file file_t;

enum seek_mode {
    // seek relative to the beginning of the file
    FSEEK_BEGIN,
    // seek relative to the position the file is currently at
    FSEEK_CURRENT,
};

struct fsdev {
    file_t* (*open)(fsdev_t* dev, const char** path, size_t pathlen);
    int (*read)(fsdev_t* dev, file_t* file, size_t size, void* buf);
    int (*seek)(fsdev_t* dev, file_t* file, int mode, int32_t offset);
    void (*close)(fsdev_t* dev, file_t* file);
    void* priv;
};

