#include "console.h"
#include "stddef.h"

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
            con->invalidate(con, 0, 0, con->width, con->height);
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
    con->invalidate(con, con->x_pos, con->y_pos, 1, 1);
    con->x_pos++;

    if (con->x_pos > con->width) {
        con->x_pos = 0;
        con->y_pos++;
        if (con->y_pos >= con->height) {
            con->y_pos--;
            con->scroll(con);
            con->invalidate(con, 0, 0, con->width, con->height);
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
    con->invalidate(con, 0, 0, con->width, con->height);
    con->x_pos = 0;
    con->y_pos = 0;
}

void console_clear_row(console_t* con, int row)
{
    for (int x = 0; x < con->width; x++) {
        con->put_xy(con, x, row, ' ');
    }
    con->invalidate(con, row, 0, con->width, 1);
}

void console_colour(console_t* con, uint16_t colour)
{
    con->set_colour(con, colour);
}

static int chardev_putc(chardev_t* dev, int c)
{
    console_t* con = (console_t*)dev->priv;
    console_putc(con, c);
    return 1;
}

void console_get_chardev(console_t* con, chardev_t* chardev)
{
    chardev->putc = chardev_putc;
    chardev->getc = NULL;
    chardev->priv = con;
}
