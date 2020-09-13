#include "vga.h"
#include "pio.h"
#include "console.h"
#include "../alloc.h"
#include "../sys/bios.h"
#include "../kernel.h"
#include "../stdlib.h"


struct vga_state
{
    uint16_t colour;
    uint16_t* vga_buffer;
};

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

void vga_set_cursor(console_t* con, int x, int y)
{
    uint16_t pos = y * VGA_WIDTH + x;

    outb(0x3d4, 0x0f);
    outb(0x3d5, pos & 0xff);
    outb(0x3d4, 0x0e);
    outb(0x3d5, (pos >> 8) & 0xff);
}

void vga_put_xy(console_t* con, int x, int y, char c)
{
    struct vga_state* state = con->priv;
    ASSERT(con && x <= con->width && y <= con->height, "Out of bounds for screen");
    state->vga_buffer[y * con->width + x] = vga_entry(c, state->colour);
}

void vga_scroll(console_t* con)
{
    // struct vga_state* state = con->priv;
    // for(int y = 0; y < VGA_HEIGHT; y++) {
    //     for(int x = 0; x < VGA_WIDTH; x++) {
    //         state->vga_buffer[(y * VGA_WIDTH) + x] = state->vga_buffer[(y + 1) * VGA_WIDTH + x];
    //     }
    // }

    // vga_clear_row(y_pos);
}

void vga_set_mode(uint8_t mode)
{
    struct int_regs regs;
    memset(&regs, 0, sizeof(regs));

    regs.eax = mode; // ah=00, al=mode
    bios_interrupt(0x10, &regs);
}

console_t* vga_init(uint16_t colour)
{
    console_t* con = kalloc(sizeof(*con));
    con->priv = kalloc(sizeof(struct vga_state));
    
    struct vga_state* state = con->priv;
    state->vga_buffer = (uint16_t*)VGA_BUFFER_ADDR;
    state->colour = colour;

    con->put_xy = vga_put_xy;
    con->set_cursor = vga_set_cursor;
    con->scroll = vga_scroll;

    con->width = VGA_WIDTH;
    con->height = VGA_HEIGHT;

    vga_cursor_width(14, 15);

    // Clear the whole screen
    console_clear(con);

    con->x_pos = 0;
    con->y_pos = 0;

    return con;
}