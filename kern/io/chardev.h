#pragma once

typedef struct chardev chardev_t;

typedef int (*char_putc_t)(chardev_t* dev, int c);
typedef int (*char_getc_t)(chardev_t* dev);

#define EOF -1

typedef struct chardev {
    char_putc_t putc;
    char_getc_t getc;
    void* priv;
} chardev_t;
