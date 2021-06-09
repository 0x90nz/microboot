#pragma once

#include "console.h"

typedef struct {
    struct vesa_device* dev;
    uint32_t fg_colour;
    uint32_t bg_colour;
} fbcon_priv_t;

void fbcon_init(struct vesa_device* dev, console_t* con);
void fbcon_destroy(console_t* con);
