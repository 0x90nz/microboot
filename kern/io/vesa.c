/**
 * @file vesa.c
 * @brief Driver for a VBE 3.0 compliant graphics card. Uses BIOS interrupts
 * to do setting of modes and retrieval of information and has a backbuffer
 * to improve the efficiency of drawing operations.
 */

#include "vesa.h"
#include <export.h>
#include <errors.h>
#include <stdint.h>
#include "driver.h"
#include "framebuffer.h"
#include "../sys/bios.h"
#include "../stdlib.h"
#include "../sys/addressing.h"
#include "../alloc.h"

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

struct vesa_priv {
    int width, height;
    int bits_per_pixel;
    int bytes_per_pixel;
    int pitch;
    uint8_t* framebuffer;
    uint8_t* backbuffer;
};

typedef int (*vesa_mode_fn_t)(int x, int y, int bpp);

// Check that the returned value is the OK vbe status
static inline int vbe_ok(uint32_t ret)
{
    return (ret & 0xffff) == VBE_STATUS_OK;
}

/**
 * @brief Set the VBE mode number
 *
 * @param mode the mode number
 * @return int non-zero on success, zero on failure
 */
int vesa_set_mode(int mode)
{
    struct int_regs regs;
    memset(&regs, 0, sizeof(regs));
    regs.eax = 0x4f02;
    regs.ebx = 0x4000 | mode;
    bios_interrupt(0x10, &regs);

    return vbe_ok(regs.eax);
}

/**
 * @brief Get the current VBE mode number
 *
 * @return int the current VBE mode number, -1 if failure
 */
int vesa_get_mode()
{
    struct int_regs regs;
    memset(&regs, 0, sizeof(regs));
    regs.eax = 0x4f03;
    bios_interrupt(0x10, &regs);

    return vbe_ok(regs.eax) ? regs.ebx & 0x1fff : -1;
}

/**
 * @brief Get the mode information for a given VBE mode
 *
 * @param mode the mode number
 * @param info where to store the resulting mode info block
 * @return int non-zero value on success, zero on failure
 */
int vesa_get_info(int mode, struct vbe_mode_info_block* info)
{
    struct int_regs regs;
    memset(&regs, 0, sizeof(regs));

    regs.es = SEGOF((uint32_t)(&low_mem_buffer));
    regs.edi = OFFOF(((uint32_t)&low_mem_buffer));
    regs.eax = 0x4f01;
    regs.ecx = mode;

    bios_interrupt(0x10, &regs);

    if (vbe_ok(regs.eax)) {
        memcpy(info, &low_mem_buffer, sizeof(struct vbe_mode_info_block));
        return 1;
    } else {
        return 0;
    }
}

/**
 * @brief Get thhe VESA info block
 *
 * @param info pointer to struct to copy info into
 * @return int non-zero on success, zero on failure
 */
int vesa_get_info_block(struct vbe_info_block* info)
{
    struct int_regs regs;
    memset(&regs, 0, sizeof(regs));

    struct vbe_info_block* low_info = (struct vbe_info_block*)&low_mem_buffer;
    memset(low_info, 0, sizeof(*low_info));
    memcpy(low_info->sig, "VBE2", 4);

    regs.es = SEGOF(low_info);
    regs.edi = OFFOF(low_info);
    regs.eax = 0x4f00;
    bios_interrupt(0x10, &regs);

    if (vbe_ok(regs.eax)) {
        memcpy(info, low_info, sizeof(*info));
        return 1;
    } else {
        return 0;
    }
}

int vesa_find_mode(int x, int y, int bpp, vesa_mode_fn_t mode_ok)
{
    struct vbe_info_block* info = kalloc(4096);
    memset(info, 0, 4096);
    if (!vesa_get_info_block(info)) {
        log(LOG_WARN, "FAIL: VESA not supported");
        return -1;
    }

    // Find how far into the buffer the mode info starts
    int mode_off = U32REAL_TO_LIN(uint8_t*, info->video_mode_ptr) - &low_mem_buffer;
    debugf("OEM product name: %s", U32REAL_TO_LIN(const char*, info->oem_product_name_ptr));
    debugf("OEM vendor name: %s", U32REAL_TO_LIN(const char*, info->oem_vendor_name_ptr));

    uint16_t mode_buffer[4096];
    memcpy(mode_buffer, (uint8_t*)info + mode_off, 4096 - mode_off);
    kfree(info);

    struct vbe_mode_info_block mib;

    for (int i = 0; i < (4096 - mode_off) && mode_buffer[i] != 0xFFFF; i++) {
        if (vesa_get_info(mode_buffer[i], &mib)) {
            if (mib.mode_attrs & 0x90 && mib.memory_model == 6) {
                if (mib.x_res == x && mib.y_res == y && mib.bits_per_pixel == bpp)
                    return mode_buffer[i];
                else if (mode_ok && mode_ok(mib.x_res, mib.y_res, mib.bits_per_pixel))
                    return mode_buffer[i];
            }
        }
    }

    return -1;
}


int vesa_set_res(struct vesa_priv* device, int x, int y, int bpp)
{
    int mode = vesa_find_mode(x, y, bpp, NULL);
    if (mode == -1) {
        return -ERR_INVALID;
    }

    logf(LOG_INFO, "used mode %d", mode);

    struct vbe_mode_info_block info;

    vesa_get_info(mode, &info);
    if (!vesa_set_mode(mode))
        return -ERR_INVALID;

    if (device->backbuffer)
        kfree(device->backbuffer);

    device->framebuffer = (uint8_t*)info.phys_base_ptr;
    device->width = info.x_res;
    device->height = info.y_res;
    device->bits_per_pixel = info.bits_per_pixel;
    device->bytes_per_pixel = device->bits_per_pixel / 8;
    device->pitch = info.lin_pitch;
    device->backbuffer = kallocz(info.x_res * info.y_res * device->bytes_per_pixel);

    logf(
        LOG_INFO,
        "initialised with res %dx%dx%d",
        device->width, device->height,
        device->bits_per_pixel
    );
    logf(LOG_INFO, "vesa LFB at %08x", device->framebuffer);
    return 0;
}

/**
 * @brief Set the colour value of a single pixel. No bounds checking is done
 * as this would slow down execution, this means that care should be taken
 * when calling this function.
 *
 * @param device the device to set the pixel on
 * @param x the x position of the pixel
 * @param y the y position of the pixel
 * @param color the colour to set. is format specific, e.g. a 16 bit colour
 * will not produce the same output if sent to a 32 bit vesa device.
 */
void vesa_put_pixel(fbdev_t* dev, int x, int y, uint32_t colour)
{
    struct vesa_priv* device = dev->priv;
    *(uint32_t*)&device->backbuffer[(x * device->bytes_per_pixel) + device->pitch * y] = colour;
}

/**
 * @brief Get the colour value of a single pixel
 *
 * @param device the device to get the pixel from
 * @param x the x position of the pixel
 * @param y the y position of the pixel
 * @return uint32_t the colour, note that in 8 bit modes this may include extra
 * information and should be masked to 8 bits by the caller
 */
uint32_t vesa_get_pixel(fbdev_t* dev, int x, int y)
{
    struct vesa_priv* device = dev->priv;
    return *(uint32_t*)&device->backbuffer[(x * device->bytes_per_pixel) + device->pitch * y];
}

/**
 * @brief Shift the entire screen up
 *
 * @param device the device to apply the shift to
 * @param yshift the amount of pixels to shift up vertically by
 */
void vesa_shift(fbdev_t* dev, int yshift)
{
    struct vesa_priv* device = dev->priv;
    for (int y = 0; y < device->height - yshift; y++) {
        int src_offset = device->pitch * y;
        int dst_offset = device->pitch * (y + yshift);
        memcpy(device->backbuffer + src_offset, device->backbuffer + dst_offset, device->pitch);
    }
}

/**
 * @brief Invalidate an area of the buffer, copying its contents from the back
 * buffer of this device to the framebuffer.
 *
 * @param device the device to perform invalidation on
 * @param x_start the start x coordinate for invalidation
 * @param y_start the start y coordinate for invalidation
 * @param width the width that the invalidated rectangle should be
 * @param height the height that the invalidated rectangle should be
 */
void vesa_invalidate(fbdev_t* dev, int x_start, int y_start, int width, int height)
{
    struct vesa_priv* device = dev->priv;
    // TODO: some faster way of doing this?
    switch (device->bits_per_pixel) {
    case 8:
        for (int y = y_start; y < (y_start + height); y++) {
            for (int x = x_start; x < (x_start + width); x++) {
                int offset = x * device->pitch * y;
                device->framebuffer[offset] = device->backbuffer[offset];
            }
        }
        break;

    default:
    case 24:
    case 32:
        for (int y = y_start; y < (y_start + height); y++) {
            for (int x = x_start; x < (x_start + width); x++) {
                int offset = (x * device->bytes_per_pixel) + device->pitch * y;
                *(uint32_t*)&device->framebuffer[offset] = *(uint32_t*)&device->backbuffer[offset];
            }
        }
        break;
    }
}

/**
 * @brief Destroy a vesa device, freeing any resources allocated to it
 *
 * @param device the device to destroy
 */
void vesa_destroy(struct device* dev)
{
    fbdev_t* fbdev = dev->internal_dev;
    struct vesa_priv* device = fbdev->priv;
    kfree(device->backbuffer);
    kfree(fbdev->priv);
    kfree(fbdev);
}

int vesa_setparam(struct device* dev, int paramid, void* aux)
{
    fbdev_t* fbdev = dev->internal_dev;
    struct vesa_priv* priv = fbdev->priv;

    if (paramid == FBDEV_SETRES) {
        if (device_deregister_subdevices(dev)) {
            kfree(dev->subdevices);
            dev->subdevices = NULL;
        }

        struct fbdev_setres_request* req = aux;
        int err = vesa_set_res(priv, req->xres, req->yres, req->bpp);
        if (err < 0)
            return err;

        fbdev->width = priv->width;
        fbdev->height = priv->height;
        fbdev->bpp = priv->bits_per_pixel;

        // we got rid of any users so bring them back with a probe
        driver_probe_for(DEVICE_TYPE_CON);
        return 0;
    }

    return -ERR_INVALID;
}

static void vesa_probe(struct driver* driver)
{
    if (!driver->first_probe)
        return;

    // no vesa for us
    if (vesa_get_mode() == -1)
        return;

    struct vesa_priv* vesa_priv = kallocz(sizeof(*vesa_priv));
    fbdev_t* fbdev = kalloc(sizeof(*fbdev));
    fbdev->priv = vesa_priv;
    fbdev->get_pixel = vesa_get_pixel;
    fbdev->invalidate = vesa_invalidate;
    fbdev->put_pixel = vesa_put_pixel;
    fbdev->shift = vesa_shift;

    struct device* dev = kalloc(sizeof(*dev));
    dev->internal_dev = fbdev;
    dev->type = DEVICE_TYPE_FRAMEBUFFER;
    sprintf(dev->name, "vesafb%d", device_get_first_available_suffix("vesafb"));
    dev->destroy = vesa_destroy;
    dev->device_priv = NULL;
    dev->setparam = vesa_setparam;

    device_register(dev);
}

struct driver vesa_driver = {
    .name = "VBE 3.0 framebuffer",
    .probe = vesa_probe,
    .type_for = DEVICE_TYPE_FRAMEBUFFER,
    .driver_priv = NULL,
};

static void vesa_register_driver()
{
    driver_register(&vesa_driver);
}
EXPORT_INIT(vesa_register_driver);
