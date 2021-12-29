#pragma once

#include <stddef.h>

typedef struct fsdev fsdev_t;
typedef struct file file_t;

struct fsdev {
    file_t* (*open)(fsdev_t* dev, const char** path, size_t pathlen);
    void (*close)(fsdev_t* dev, file_t* file);
    void* priv;
};

