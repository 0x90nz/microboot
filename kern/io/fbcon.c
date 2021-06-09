#include "fbcon.h"
#include "vesa.h"
#include "../alloc.h"
#include "../res/cherry_font.h"
#include "../stdlib.h"

#define CHAR_HEIGHT 13
#define CHAR_WIDTH 7

static void fb_putxy(console_t* con, int x_base, int y_base, char c)
{
    // A mask is theroetically faster than bitshift operations.
    // TODO: check that is actually true
    fbcon_priv_t* priv = con->priv;
    struct vesa_device* dev = priv->dev;
    int mask[] = {128, 64, 32, 16, 8, 4, 2, 1};
    int px_x = x_base * CHAR_WIDTH;
    int px_y = y_base * CHAR_HEIGHT;
    for (int y = 0; y < CHAR_HEIGHT; y++) {
        for (int x = 0; x < CHAR_WIDTH; x++) {
            if (font_cherry[(unsigned char)c - 32][y] & mask[x]) {
                vesa_put_pixel(dev, px_x + x, px_y + y, priv->fg_colour);
            } else {
                vesa_put_pixel(dev, px_x + x, px_y + y, priv->bg_colour);
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
    struct vesa_device* dev = priv->dev;
    // TODO: replace with bulk copy?
    vesa_shift(dev, CHAR_HEIGHT);
    console_clear_row(con, con->y_pos);
}

static void fb_invalidate(console_t* con, int x, int y, int width, int height)
{
    fbcon_priv_t* priv = con->priv;
    vesa_invalidate(
        priv->dev,
        x * CHAR_WIDTH, y * CHAR_HEIGHT,
        width * CHAR_WIDTH, height * CHAR_HEIGHT
    );
}

static void fb_null() {}

void fbcon_init(struct vesa_device* dev, console_t* con)
{
    fbcon_priv_t* priv = kalloc(sizeof(fbcon_priv_t));
    priv->dev = dev;
    priv->fg_colour = 0xffffff;
    priv->bg_colour = 0;
    con->priv = priv;

    con->width = dev->width / CHAR_WIDTH;
    con->height = dev->height / CHAR_HEIGHT;

    con->put_xy = fb_putxy;
    con->set_cursor = fb_null;
    con->set_colour = fb_set_colour;
    con->invalidate = fb_invalidate;
    con->scroll = fb_scroll;
}

void fbcon_destroy(console_t* con)
{
    kfree(con->priv);
}
