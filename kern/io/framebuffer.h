#pragma once

#include <stdint.h>

typedef uint32_t fbcolour_t;
typedef struct framebuffer_device fbdev_t;

typedef struct framebuffer_device {
    // TODO: bulk writes
    void (*put_pixel)(fbdev_t* dev, int x, int y, fbcolour_t colour);
    fbcolour_t (*get_pixel)(fbdev_t* dev, int x, int y);
    void (*invalidate)(fbdev_t* dev, int x, int y, int width, int height);
    void (*shift)(fbdev_t* dev, int height);

    int width;
    int height;
    int bpp;

    void* priv;
} fbdev_t;

enum vesa_setparam {
    FBDEV_SETRES
};

struct fbdev_setres_request {
    int xres;
    int yres;
    int bpp;
};

