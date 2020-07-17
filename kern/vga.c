#include "vga.h"
#include "pio.h"
#include "kernel.h"

uint16_t* vga_buffer;
int x_pos;
int y_pos;
uint16_t default_colour;

uint16_t vga_entry(uint8_t c, uint8_t colour)
{
    return c | (colour << 8);
}

uint8_t vga_colour(uint8_t fg, uint8_t bg)
{
    return fg | bg << 4;
}

void vga_disable_cursor()
{
    outb(0x3d4, 0x0a);
    outb(0x3d5, 0x20);
}

void vga_cursor_width(uint8_t start, uint8_t end)
{
    outb(0x3d4, 0x0a);
    outb(0x3d5, (inb(0x3d5) & 0xc0) | start);
    outb(0x3d4, 0x0b);
    outb(0x3d5, (inb(0x3d5) & 0xe0) | end);
}

void vga_set_cursor(int x, int y)
{
    uint16_t pos = y * VGA_WIDTH + x;

    outb(0x3d4, 0x0f);
    outb(0x3d5, pos & 0xff);
    outb(0x3d4, 0x0e);
    outb(0x3d5, (pos >> 8) & 0xff);
}


void vga_clear_row(int row)
{
    for (int x = 0; x < VGA_WIDTH; x++) 
    {
        vga_buffer[row * VGA_WIDTH + x] = vga_entry(' ', default_colour);
    }
}

void vga_scroll()
{
    for(int y = 0; y < VGA_HEIGHT; y++)
    {
        for(int x = 0; x < VGA_WIDTH; x++)
        {
            vga_buffer[(y * VGA_WIDTH) + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }

    vga_clear_row(y_pos);
}

void vga_erase()
{
    if (x_pos == 0)
    {
        if (y_pos > 0)
        {
            y_pos--;
            x_pos = 0;
        }
    }
    else if (x_pos > 0)
    {
        x_pos--;
    }
    vga_putc(' ');
    x_pos--;
}

void vga_putc(unsigned char c)
{
    if (c == '\n')
    {
        x_pos = 0;
        y_pos++;
        if (y_pos >= VGA_HEIGHT)
        {
            y_pos--;
            vga_scroll();
        }
        vga_set_cursor(x_pos, y_pos);
        return;
    }
    else if (c == '\b')
    {
        vga_erase();
        vga_set_cursor(x_pos, y_pos);
        return;
    }
    else if(c == '\r')
    {
        x_pos = 0;
        vga_set_cursor(x_pos, y_pos);
        return;
    }

    vga_buffer[y_pos * VGA_WIDTH + x_pos] = vga_entry(c, default_colour);
    x_pos++;

    if (x_pos > VGA_WIDTH)
    {
        x_pos = 0;
        y_pos++;
        if (y_pos >= VGA_HEIGHT)
        {
            y_pos--;
            vga_scroll();
        }
    }

    vga_set_cursor(x_pos, y_pos);
}


void vga_pad(int n)
{
    for (int i = 0; i < n; i++)
        vga_putc(' ');
}

void vga_puts(const char* str)
{
    char* s = (char*)str;
    while (*s != '\0')
    {
        vga_putc(*s);
        s++;
    }
}

void vga_init(uint16_t colour)
{
    vga_buffer = (uint16_t*)VGA_BUFFER_ADDR;
    default_colour = colour;

    vga_cursor_width(14, 15);

    // Clear the whole screen
    for (int y = 0; y < VGA_HEIGHT; y++)
    {
        for (int x = 0; x < VGA_WIDTH; x++)
        {
            vga_buffer[y * VGA_WIDTH + x] = vga_entry(' ', default_colour);
        }
    }

    x_pos = 0;
    y_pos = 0;
}

uint16_t vga_get_default()
{
    return default_colour;
}