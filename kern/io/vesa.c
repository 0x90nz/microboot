/**
 * @file vesa.c
 * @brief Driver for a VBE 3.0 compliant graphics card. Uses BIOS interrupts
 * to do setting of modes and retrieval of information and has a backbuffer
 * to improve the efficiency of drawing operations.
 */

#include "vesa.h"
#include "../sys/bios.h"
#include "../stdlib.h"
#include "../sys/addressing.h"
#include "../alloc.h"

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


/**
 * @brief Initialise a vesa device from a given mode info block
 *
 * @param device the device to initialise
 * @param info the mode info block describing the mode the devices mode
 */
void vesa_raw_init(struct vesa_device* device, struct vbe_mode_info_block* info)
{
    device->framebuffer = (uint8_t*)info->phys_base_ptr;
    device->width = info->x_res;
    device->height = info->y_res;
    device->bits_per_pixel = info->bits_per_pixel;
    device->bytes_per_pixel = device->bits_per_pixel / 8;
    device->pitch = info->lin_pitch;
    device->backbuffer = kallocz(info->x_res * info->y_res * info->bits_per_pixel);

    logf(
        LOG_INFO,
        "initialised with res %dx%dx%d",
        device->width, device->height,
        device->bits_per_pixel
    );
    logf(LOG_INFO, "vesa LFB at %08x", device->framebuffer);
}

int vesa_init(struct vesa_device* device, int x, int y, int bpp)
{
    int mode = vesa_find_mode(x, y, bpp, NULL);
    if (mode == -1) {
        return 0;
    }

    logf(LOG_INFO, "init used mode %d", mode);

    struct vbe_mode_info_block mib;

    vesa_get_info(mode, &mib);
    vesa_set_mode(mode);
    vesa_raw_init(device, &mib);
    return 0;
}

/**
 * @brief Destroy a vesa device, freeing any resources allocated to it
 *
 * @param device the device to destroy
 */
void vesa_destroy(struct vesa_device* device)
{
    kfree(device->backbuffer);
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
void vesa_put_pixel(struct vesa_device* device, int x, int y, uint32_t colour)
{
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
uint32_t vesa_get_pixel(struct vesa_device* device, int x, int y)
{
    return *(uint32_t*)&device->backbuffer[(x * device->bytes_per_pixel) + device->pitch * y];
}

/**
 * @brief Shift the entire screen up
 *
 * @param device the device to apply the shift to
 * @param yshift the amount of pixels to shift up vertically by
 */
void vesa_shift(struct vesa_device* device, int yshift)
{
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
void vesa_invalidate(struct vesa_device* device, int x_start, int y_start, int width, int height)
{
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
