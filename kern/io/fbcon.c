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
    int cursor_x;
    int cursor_y;
    uint8_t* cursor_bitmap;
} fbcon_priv_t;

static void fb_invalidate(console_t* con, int x, int y, int width, int height);

static void fb_put_bitmap(
        console_t* con,
        int px_x, int px_y,
        int width, int height, int wbytes,
        uint32_t fg, uint32_t bg,
        const uint8_t* bitmap) {
    // A mask is theroetically faster than bitshift operations.
    // TODO: check that is actually true
    fbcon_priv_t* priv = con->priv;
    fbdev_t* fb = priv->fb;

    const int mask[] = {128, 64, 32, 16, 8, 4, 2, 1};
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // figure out the index into the bitmap, and the specifc bit within
            // that byte we're looking at
            int idx = (y * wbytes) + (x >> 3); // x >> 3 == x / 8
            int off = x & 7; // x % 8

            if (bitmap[idx] & mask[off]) {
                fb->put_pixel(fb, px_x + x, px_y + y, fg);
            } else {
                fb->put_pixel(fb, px_x + x, px_y + y, bg);
            }
        }
    }
}

// fb->put_pixel(fb, px_x + x, px_y + y, priv->bg_colour);
static void fb_putxy(console_t* con, int x_base, int y_base, char c)
{
    fbcon_priv_t* priv = con->priv;

    // if we're going to be overwriting where the cursor was, then stop
    // tracking the old cursor position
    if (priv->cursor_x == x_base && priv->cursor_y == y_base) {
        priv->cursor_x = -1;
        priv->cursor_y = -1;
    }

    const int px_x = x_base * priv->font.char_width;
    const int px_y = y_base * priv->font.char_height;

    // uint8_t* bitmap = priv->font.data + ((c - 32) * priv->font.char_height);
    const int char_height = priv->font.char_height;
    const int char_width = priv->font.char_width;
    const int char_wbytes = priv->font.char_width_bytes;
    const uint32_t fg = priv->fg_colour;
    const uint32_t bg = priv->bg_colour;

    const uint8_t* bitmap = priv->font.data + (char_height * char_wbytes * (c - 32));
    fb_put_bitmap(
            con,
            px_x, px_y,
            char_width, char_height, char_wbytes,
            fg, bg,
            bitmap
    );
}

void fb_set_cursor(console_t* con, int x, int y)
{
    fbcon_priv_t* priv = con->priv;
    int old_x = priv->cursor_x;
    int old_y = priv->cursor_y;

    if (old_x != -1 && old_y != -1) {
        fb_putxy(con, old_x, old_y, ' ');
        fb_invalidate(con, old_x, old_y, 1, 1);
    }

    const int px_x = x * priv->font.char_width;
    const int px_y = y * priv->font.char_height;
    fb_put_bitmap(
            con,
            px_x, px_y,
            priv->font.char_width, priv->font.char_height, priv->font.char_width_bytes,
            priv->fg_colour, priv->bg_colour,
            priv->cursor_bitmap
    );

    // fb_putxy(con, x, y, '_');
    fb_invalidate(con, x, y, 1, 1);

    priv->cursor_x = x;
    priv->cursor_y = y;
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

void fbcon_destroy(struct device* dev)
{
    console_t* con = dev->internal_dev;
    kfree(con->priv);
    kfree(con);
    kfree(dev);
}

static void fbcon_gencursor(console_t* con)
{
    fbcon_priv_t* priv = con->priv;
    if (priv->cursor_bitmap)
        kfree(priv->cursor_bitmap);

    struct fbcon_font* font = &priv->font;
    uint8_t* bmp = kallocz(font->char_width_bytes * font->char_height);
    for (int i = 0; i < font->char_height; i++) {
        bmp[i * font->char_width_bytes] = 0x40;
    }

    priv->cursor_bitmap = bmp;
}

static void fbcon_resize(console_t* con)
{
    fbcon_priv_t* priv = con->priv;
    fbdev_t* fb = priv->fb;

    int old_width = con->width;
    int old_height = con->height;

    // setup dimensions to match any changes to fb
    con->width = fb->width / priv->font.char_width;
    con->height = fb->height / priv->font.char_height;

    debugf("fbcon changing to %d, %d", fb->width, fb->height);
    debugf("meaning char width %d, char height %d", con->width, con->height);

    if (con->width == old_width && con->height == old_height) {
        // nothing to do here, the dimensions didn't actually change
        return;
    }

    // re-init screen to known state
    console_clear(con);
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
       fbcon_gencursor(con);
       console_clear(con);
       break;
    }
    return 0;
}

int fbcon_inform(struct device* dev, struct device* sender, enum change_type type, int id, void* aux) {
    debugf("got inform change %d, id %d", type, id);
    if (sender->type == DEVICE_TYPE_FRAMEBUFFER) {
        if (type == CHANGE_TYPE_PARAM && id == FBDEV_CHANGE_RESOLUTION) {
            console_t* con = device_get_console(dev);
            fbcon_resize(con);
        }
    }
    return 0;
}

static struct device* fbcon_create(fbdev_t* fb)
{
    console_t* con = kalloc(sizeof(*con));
    fbcon_priv_t* priv = kalloc(sizeof(*priv));
    con->priv = priv;

    priv->fb = fb;

    priv->font.data = (uint8_t*)font_cherry;
    priv->font.char_width = 7;
    priv->font.char_width_bytes = 1;
    priv->font.char_height = 13;
    priv->cursor_bitmap = NULL;
    fbcon_gencursor(con);

    memcpy(priv->colour_map, base_colour_map, sizeof(base_colour_map));
    priv->fg_colour = priv->colour_map[COLOUR_DEFAULT_FG];
    priv->bg_colour = priv->colour_map[COLOUR_DEFAULT_BG];
    priv->cursor_x = -1;
    priv->cursor_y = -1;

    con->width = fb->width / priv->font.char_width;
    con->height = fb->height / priv->font.char_height;
    con->put_xy = fb_putxy;

    con->set_cursor = fb_set_cursor;
    con->set_colour = fb_set_colour;
    con->invalidate = fb_invalidate;
    con->clear = fb_clear;
    con->scroll = fb_scroll;

    struct device* dev = kallocz(sizeof(*dev));
    dev->destroy = fbcon_destroy;
    dev->setparam = fbcon_setparam;
    dev->inform = fbcon_inform;
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
    debugf("%s", dev->name);
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

            // We registered a new console, get rid of the old one!!
            struct device* vga = device_get_by_name("vga0"); // TODO: get properly
            if (vga)
                device_deregister(vga);
            console = device_get_console(new_con);
            kfree(stdout);
            stdout = kalloc(sizeof(*stdout));
            console_get_chardev(console, stdout);
        }
    }
}

static void fbcon_probe(struct driver* driver, struct device* invoker)
{
    debug("fbcon probe");
    device_foreach(fbcon_dev_foreach_callback);
}

struct driver fbcon_driver = {
    .name = "Framebuffer console",
    .probe_directed = fbcon_probe,
    .type_for = DEVICE_TYPE_CON,
    .depends_on = DEVICE_TYPE_FRAMEBUFFER,
    .driver_priv = NULL,
};

static void fbcon_register_driver()
{
    driver_register(&fbcon_driver);
}
EXPORT_INIT(fbcon_register_driver);

