#pragma once
#include <stdint.h>
#include "chardev.h"

// All of these operations should be implemented, but some are not required
// to do anything
typedef struct console_state
{
    // Put a character at an x and y position
    void (*put_xy)(struct console_state*, int, int, char);
    // Set the position of the console cursor
    void (*set_cursor)(struct console_state*, int, int);
    // Scroll the console up one row
    void (*scroll)(struct console_state*);
    // Set the colour that characters put after this call will be drawn as
    void (*set_colour)(struct console_state*, uint16_t);
    // Called to flush a set of operations to the display. Changes /may/ be
    // written to display before invalidate is called, or it may be entirely
    // ignored. It is just a hint to the console.
    void (*invalidate)(struct console_state* con, int x, int y, int width, int height);
    // Called before invalidate on a clear
    void (*clear)(struct console_state* con);

    int x_pos, y_pos;
    int width, height;
    void* priv;
} console_t;

void console_putc(console_t* con, char c);
void console_erase(console_t* con);
void console_pad(console_t* con, int n);
void console_clear(console_t* con);
void console_clear_row(console_t* con, int row);
void console_colour(console_t* con, uint16_t colour);
void console_get_chardev(console_t* con, chardev_t* chardev);

enum console_colour {
    COLOUR_BLACK,
    COLOUR_BLUE,
    COLOUR_GREEN,
    COLOUR_CYAN,
    COLOUR_RED,
    COLOUR_PURPLE,
    COLOUR_BROWN,
    COLOUR_GRAY,
    COLOUR_DARK_GRAY,
    COLOUR_LIGHT_BLUE,
    COLOUR_BRIGHT_GREEN,
    COLOUR_LIGHT_CYAN,
    COLOUR_LIGHT_RED,
    COLOUR_LIGHT_PURPLE,
    COLOUR_YELLOW,
    COLOUR_WHITE,
};

#define COLOUR_DEFAULT_FG       COLOUR_WHITE
#define COLOUR_DEFAULT_BG       COLOUR_BLUE

