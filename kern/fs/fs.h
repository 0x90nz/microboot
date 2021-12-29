#pragma once

#include <stdint.h>
#include <stddef.h>
#include "../io/fsdev.h"

#define FS_PATH_SEPARATOR       "/"
#define FS_PATH_SEPARATOR_CHAR  '/'
#define FS_DEV_SEPARATOR        "$"
#define FS_DEV_SEPARATOR_CHAR   '$'

typedef struct filehandle filehandle_t;

filehandle_t* fs_open(const char* path);
void fs_close(filehandle_t* file);

