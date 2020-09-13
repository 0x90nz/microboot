#include "console.h"

/**
 * @brief Write a single character to the console device
 * 
 * @param con the console to write to
 * @param c the character to write
 */
void console_putc(console_t* con, char c)
{
    if (c == '\n') {
        con->x_pos = 0;
        con->y_pos++;
        if (con->y_pos >= con->height) {
            con->y_pos--;
            con->scroll(con);
        }
        con->set_cursor(con, con->x_pos, con->y_pos);
        return;
    } else if (c == '\b') {
        console_erase(con);
        con->set_cursor(con, con->x_pos, con->y_pos);
        return;
    } else if(c == '\r') {
        con->x_pos = 0;
        con->set_cursor(con, con->x_pos, con->y_pos);
        return;
    }

    con->put_xy(con, con->x_pos, con->y_pos, c);
    con->x_pos++;

    if (con->x_pos > con->width) {
        con->x_pos = 0;
        con->y_pos++;
        if (con->y_pos >= con->height) {
            con->y_pos--;
            con->scroll(con);
        }
    }

    con->set_cursor(con, con->x_pos, con->y_pos);
}

void console_erase(console_t* con)
{
    if (con->x_pos == 0) {
        if (con->y_pos > 0) {
            con->y_pos--;
            con->x_pos = 0;
        }
    } else if (con->x_pos > 0) {
        con->x_pos--;
    }
    console_putc(con, ' ');
    con->x_pos--;
}

void console_pad(console_t* con, int n)
{
    for (int i = 0; i < n; i++)
        console_putc(con, ' ');
}

void console_clear(console_t* con)
{
    for (int y = 0; y < con->height; y++) {
        for (int x = 0; x < con->width; x++) {
            con->put_xy(con, x, y, ' ');
        }
    }
    con->x_pos = 0;
    con->y_pos = 0;
}

void console_clear_row(console_t* con, int row)
{
    for (int x = 0; x < con->width; x++) {
        con->put_xy(con, x, row, ' ');
    }
}