#pragma once

typedef struct console_state
{
    void (*put_xy)(struct console_state*, int, int, char);
    void (*set_cursor)(struct console_state*, int, int);
    void (*scroll)(struct console_state*);

    int x_pos, y_pos;
    int width, height;
    void* priv;
} console_t;

void console_putc(console_t* con, char c);
void console_erase(console_t* con);
void console_pad(console_t* con, int n);
void console_clear(console_t* con);