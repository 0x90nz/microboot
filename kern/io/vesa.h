#pragma once

#include <stdint.h>

#define VBE_STATUS_OK       0x004f

struct vbe_info_block {
    uint8_t sig[4];
    uint16_t version;

    uint32_t oem_string_ptr;
    uint32_t capabilities;
    uint32_t video_mode_ptr;
    uint16_t total_memory;

    uint16_t oem_software_rev;
    uint32_t oem_vendor_name_ptr;
    uint32_t oem_product_name_ptr;
    uint32_t oem_product_rev_ptr;

    uint8_t reserved[222];
    uint8_t oem_data[256];
} __attribute__((packed));


struct vbe_mode_info_block {
    uint16_t mode_attrs;
    uint8_t win_a_attrs, win_b_attrs;
    uint16_t win_granularity;
    uint16_t win_size;
    uint16_t win_a_segment, win_b_segment;
    uint32_t win_func_ptr;
    uint16_t pitch; // aka bytes per scanline

    // VBE >= 1.2
    uint16_t x_res, y_res;
    uint8_t x_char_size, y_char_size;
    uint8_t num_planes;
    uint8_t bits_per_pixel;
    uint8_t num_banks;
    uint8_t memory_model;
    uint8_t bank_size;
    uint8_t num_image_pages;
    uint8_t reserved0;

    uint8_t red_mask_size, red_field_position;
    uint8_t green_mask_size, green_field_position;
    uint8_t blue_mask_size, blue_field_position;
    uint8_t resvd_mask_size, resvd_mask_position;
    uint8_t direct_color_mode_info;

    // VBE >= 2.0
    uint32_t phys_base_ptr;
    uint32_t reserved1;
    uint16_t reserved2;

    // VBE >= 3.0
    uint16_t lin_pitch;
    uint8_t bank_num_image_pages;
    uint8_t lin_num_image_pages;
    uint8_t lin_red_mask_size, lin_red_field_position;
    uint8_t lin_green_mask_size, lin_green_field_position;
    uint8_t lin_blue_mask_size, lin_blue_field_position;
    uint8_t lin_resvd_mask_size, lin_resvd_mask_position;
    uint32_t max_pixel_clock;
    
    uint8_t reserved3[190];
} __attribute__((packed));

struct vesa_device {
    int width, height;
    int bits_per_pixel;
    int bytes_per_pixel;
    int pitch;
    uint8_t* framebuffer;
    uint8_t* backbuffer;
};

typedef int (*vesa_mode_fn_t)(int, int, int);

int vesa_set_mode(int mode);
int vesa_get_info(int mode, struct vbe_mode_info_block* info);


int vesa_init(struct vesa_device* device, int x, int y, int bpp);
void vesa_destroy(struct vesa_device* device);

void vesa_put_pixel(struct vesa_device* device, int x, int y, uint32_t color);
uint32_t vesa_get_pixel(struct vesa_device* device, int x, int y);
void vesa_invalidate(struct vesa_device* device, int x_start, int y_start, int width, int height);
void vesa_shift(struct vesa_device* device, int yshift);
