#include "fbcon.h"
#include <export.h>
#include "framebuffer.h"
#include "driver.h"
#include "../alloc.h"
#include "../res/cherry_font.h"
#include "../stdlib.h"

typedef struct {
    fbdev_t* fb;
    uint32_t fg_colour;
    uint32_t bg_colour;
} fbcon_priv_t;

#define CHAR_HEIGHT 13
#define CHAR_WIDTH 7

static void fb_putxy(console_t* con, int x_base, int y_base, char c)
{
    // A mask is theroetically faster than bitshift operations.
    // TODO: check that is actually true
    fbcon_priv_t* priv = con->priv;
    fbdev_t* fb = priv->fb;
    int mask[] = {128, 64, 32, 16, 8, 4, 2, 1};
    int px_x = x_base * CHAR_WIDTH;
    int px_y = y_base * CHAR_HEIGHT;
    for (int y = 0; y < CHAR_HEIGHT; y++) {
        for (int x = 0; x < CHAR_WIDTH; x++) {
            if (font_cherry[(unsigned char)c - 32][y] & mask[x]) {
                fb->put_pixel(fb,  px_x + x, px_y + y, priv->fg_colour);
            } else {
                fb->put_pixel(fb, px_x + x, px_y + y, priv->bg_colour);
            }
        }
    }
}

uint32_t colour_map[] = {
    0,          // black
    0x0000ff,   // blue
    0x00ff00,   // green
    0x008b8b,   // cyan
    0xff0000,   // red
    0x800080,   // purple
    0x964b00,   // brown
    0x808080,   // gray
    0x696969,   // dark gray
    0x87cefa,   // light blue
    0x90ee90,   // light green
    0x00ffff,   // light cyan
    0xfa8072,   // light red
    0xff00ff,   // light purple
    0xffff00,   // yellow
    0xffffff,   // white
};

static void fb_set_colour(console_t* con, uint16_t colour)
{
    uint8_t fg = colour & 0xf;
    uint8_t bg = (colour >> 4) & 0xf;

    fbcon_priv_t* priv = con->priv;
    priv->bg_colour = colour_map[bg];
    priv->fg_colour = colour_map[fg];
}

static void fb_scroll(console_t* con)
{
    fbcon_priv_t* priv = con->priv;
    fbdev_t* fb = priv->fb;
    // TODO: replace with bulk copy?
    fb->shift(fb, CHAR_HEIGHT);
    console_clear_row(con, con->y_pos);
}

static void fb_invalidate(console_t* con, int x, int y, int width, int height)
{
    fbcon_priv_t* priv = con->priv;
    fbdev_t* fb = priv->fb;
    fb->invalidate(
        fb,
        x * CHAR_WIDTH, y * CHAR_HEIGHT,
        width * CHAR_WIDTH, height * CHAR_HEIGHT
    );
}

static void fb_null() {}

void fbcon_destroy(struct device* dev)
{
    console_t* con = dev->internal_dev;
    kfree(con->priv);
    kfree(con);
    kfree(dev);
}

static struct device* fbcon_create(fbdev_t* fb)
{
    console_t* con = kalloc(sizeof(*con));
    fbcon_priv_t* priv = kalloc(sizeof(*priv));

    priv->fb = fb;
    priv->fg_colour = 0xffffff;
    priv->bg_colour = 0;
    con->priv = priv;

    con->width = fb->width / CHAR_WIDTH;
    con->height = fb->height / CHAR_HEIGHT;

    con->put_xy = fb_putxy;
    con->set_cursor = fb_null;
    con->set_colour = fb_set_colour;
    con->invalidate = fb_invalidate;
    con->scroll = fb_scroll;

    struct device* dev = kalloc(sizeof(*dev));
    dev->destroy = fbcon_destroy;
    dev->device_priv = NULL;
    dev->internal_dev = con;
    sprintf(dev->name, "fbcon%d", device_get_first_available_suffix("fbcon"));
    dev->num_subdevices = 0;
    dev->subdevices = NULL;
    dev->type = DEVICE_TYPE_CON;

    return dev;
}

static void fbcon_dev_foreach_callback(struct device* dev)
{
    if (dev->type == DEVICE_TYPE_FRAMEBUFFER) {
        // this dev is already bound to something, so don't bother binding it
        if (dev->subdevices)
            return;

        fbdev_t* fb = dev->internal_dev;
        if (fb->width > 0 && fb->height > 0) {
            struct device* new_con = fbcon_create(fb);
            dev->subdevices = kalloc(sizeof(*dev->subdevices) * 1);
            dev->subdevices[0] = new_con;
            dev->num_subdevices = 1;
            device_register(new_con);
        }
    }
}

static void fbcon_probe(struct driver* driver)
{
    debug("fbcon probe");
    device_foreach(fbcon_dev_foreach_callback);
}

struct driver fbcon_driver = {
    .name = "Framebuffer console",
    .probe = fbcon_probe,
    .type_for = DEVICE_TYPE_CON,
    .driver_priv = NULL,
};

static void fbcon_register_driver()
{
    driver_register(&fbcon_driver);
}
EXPORT_INIT(fbcon_register_driver);
