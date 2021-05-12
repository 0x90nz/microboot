#pragma once
#include <stdint.h>
#include "chardev.h"

typedef struct console_state
{
    void (*put_xy)(struct console_state*, int, int, char);
    void (*set_cursor)(struct console_state*, int, int);
    void (*scroll)(struct console_state*);
    void (*set_colour)(struct console_state*, uint16_t);

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

#define COLOUR_BLACK           0
#define COLOUR_BLUE            1
#define COLOUR_GREEN           2
#define COLOUR_CYAN            3
#define COLOUR_RED             4
#define COLOUR_PURPLE          5
#define COLOUR_BROWN           6
#define COLOUR_GRAY            7
#define COLOUR_DARK_GRAY       8
#define COLOUR_LIGHT_BLUE      9
#define COLOUR_BRIGHT_GREEN    10
#define COLOUR_LIGHT_CYAN      11
#define COLOUR_LIGHT_RED       12
#define COLOUR_LIGHT_PURPLE    13
#define COLOUR_YELLOW          14
#define COLOUR_WHITE           15
