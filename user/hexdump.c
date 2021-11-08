#include <stdarg.h>
#include <export.h>
#include <printf.h>
#include <stdlib.h>
#include <alloc.h>
#include <fs/fs.h>
#include "common.h"

MODULE(hexdump);

/*
USE(printf);
USE(fs_open);
USE(fs_fread);
USE(fs_fsize);
USE(fs_fdestroy);
USE(kalloc);
USE(kfree);

void dump(fs_file_t file)
{
    size_t size = fs_fsize(file);
    char* buffer = kalloc(size);
    fs_fread(file, 0, size, buffer);

    for (size_t i = 0; i < size; ) {
        printf("%08x ", i);

        for (int j = 0; j < 16; j++) {
            if (j % 8 == 0)
                printf(" ");
            if (i + j < size)
                printf(" %02x", (unsigned int)(unsigned char)buffer[i + j]);
            else
                printf("   ");
        }
        printf("\n");
        i += 16;
    }
    kfree(buffer);
}

void main(int argc, char** argv)
{
    module_init();
    INIT(printf);
    INIT(fs_open);
    INIT(fs_fread);
    INIT(fs_fsize);
    INIT(fs_fdestroy);
    INIT(kalloc);
    INIT(kfree);

    if (argc != 2) {
        printf("Usage: %s filename\n", argv[0]);
        return;
    }

    fs_file_t file = fs_open(argv[1]);
    if (file != FS_FILE_INVALID) {
        dump(file);
        fs_fdestroy(file);
    } else {
        printf("No such file: %s\n", argv[1]);
    }
}

*/

