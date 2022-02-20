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
    uint32_t colour_map[16];
    struct fbcon_font font;
} fbcon_priv_t;

// fb->put_pixel(fb, px_x + x, px_y + y, priv->bg_colour);
static void fb_putxy(console_t* con, int x_base, int y_base, char c)
{
    // A mask is theroetically faster than bitshift operations.
    // TODO: check that is actually true
    fbcon_priv_t* priv = con->priv;
    fbdev_t* fb = priv->fb;

    const int mask[] = {128, 64, 32, 16, 8, 4, 2, 1};
    const int px_x = x_base * priv->font.char_width;
    const int px_y = y_base * priv->font.char_height;

    // uint8_t* bitmap = priv->font.data + ((c - 32) * priv->font.char_height);
    const int char_height = priv->font.char_height;
    const int char_width = priv->font.char_width;
    const int char_wbytes = priv->font.char_width_bytes;
    const uint32_t fg = priv->fg_colour;
    const uint32_t bg = priv->bg_colour;

    const uint8_t* bitmap = priv->font.data + (char_height * char_wbytes * (c - 32));

    for (int y = 0; y < char_height; y++) {
        for (int x = 0; x < char_width; x++) {
            // figure out the index into the bitmap, and the specifc bit within
            // that byte we're looking at
            int idx = (y * char_wbytes) + (x >> 3); // x >> 3 == x / 8
            int off = x & 7; // x % 8

            if (bitmap[idx] & mask[off]) {
                fb->put_pixel(fb, px_x + x, px_y + y, fg);
            } else {
                fb->put_pixel(fb, px_x + x, px_y + y, bg);
            }
        }
    }
}

uint32_t base_colour_map[] = {
    0x000000,   // black
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
    priv->bg_colour = priv->colour_map[bg];
    priv->fg_colour = priv->colour_map[fg];
}

static void fb_scroll(console_t* con)
{
    fbcon_priv_t* priv = con->priv;
    fbdev_t* fb = priv->fb;
    // TODO: replace with bulk copy?
    fb->shift(fb, priv->font.char_height);
    console_clear_row(con, con->y_pos);
}

static void fb_invalidate(console_t* con, int x, int y, int width, int height)
{
    fbcon_priv_t* priv = con->priv;
    fbdev_t* fb = priv->fb;
    fb->invalidate(
        fb,
        x * priv->font.char_width, y * priv->font.char_height,
        width * priv->font.char_width, height * priv->font.char_height
    );
}

static void fb_clear(console_t* con)
{
    fbcon_priv_t* fbcon_priv = con->priv;
    fbdev_t* fb = fbcon_priv->fb;
    int width = fbcon_priv->fb->width;
    int height = fbcon_priv->fb->height;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            fb->put_pixel(fb, x, y, fbcon_priv->bg_colour);
        }
    }
    fb->invalidate(fb, 0, 0, width, height);
}

static void fb_null() {}

void fbcon_destroy(struct device* dev)
{
    console_t* con = dev->internal_dev;
    kfree(con->priv);
    kfree(con);
    kfree(dev);
}

static int fbcon_setparam(struct device* dev, int param_id, void* aux)
{
    console_t* con = device_get_console(dev);
    fbcon_priv_t* priv = con->priv;

    switch (param_id) {
    case FBCON_SETPARAM_COLOURSCHEME:
        memcpy(priv->colour_map, aux, 16 * sizeof(uint32_t));
        break;
    case FBCON_SETPARAM_FONT:
       priv->font = *(struct fbcon_font*)aux;
       con->width = priv->fb->width / priv->font.char_width;
       con->height = priv->fb->height / priv->font.char_height;
       console_clear(con);
       break;
    }
    return 0;
}

static struct device* fbcon_create(fbdev_t* fb)
{
    console_t* con = kalloc(sizeof(*con));
    fbcon_priv_t* priv = kalloc(sizeof(*priv));

    priv->fb = fb;

    priv->font.data = (uint8_t*)font_cherry;
    priv->font.char_width = 7;
    priv->font.char_width_bytes = 1;
    priv->font.char_height = 13;

    memcpy(priv->colour_map, base_colour_map, sizeof(base_colour_map));
    priv->fg_colour = priv->colour_map[COLOUR_DEFAULT_FG];
    priv->bg_colour = priv->colour_map[COLOUR_DEFAULT_BG];
    con->priv = priv;

    con->width = fb->width / priv->font.char_width;
    con->height = fb->height / priv->font.char_height;

    con->put_xy = fb_putxy;
    con->set_cursor = fb_null;
    con->set_colour = fb_set_colour;
    con->invalidate = fb_invalidate;
    con->clear = fb_clear;
    con->scroll = fb_scroll;

    struct device* dev = kallocz(sizeof(*dev));
    dev->destroy = fbcon_destroy;
    dev->setparam = fbcon_setparam;
    dev->device_priv = NULL;
    dev->internal_dev = con;
    sprintf(dev->name, "fbcon%d", device_get_first_available_suffix("fbcon"));
    dev->num_subdevices = 0;
    dev->subdevices = NULL;
    dev->type = DEVICE_TYPE_CON;

    console_clear(con);

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

