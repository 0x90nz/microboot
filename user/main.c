#include <stdlib.h>
#include <stdint.h>
#include <interrupts.h>
#include <vga.h>
#include <kernel.h>
#include <keyboard.h>

uint16_t colour;
int ticks;
const char* prompt = "# ";
char main_scratch[64];

typedef struct {
    const char* name;
    void (*fn)(void);
} command_t;

const char* colours[] = {
    "black",
    "blue",
    "green",
    "cyan",
    "red",
    "purple",
    "brown",
    "gray",
    "dgrey",
    "lblue",
    "lcyan",
    "lred",
    "lpurple",
    "yellow",
    "white"
};

int colour_from_str(const char* str)
{
    for (int i = 0; i < sizeof(colours) / sizeof(const char*); i++)
    {
        if (strcmp(colours[i], str) == 0)
            return i;
    }
    return 0;
}

void uptime()
{
    puts("up ");
    itoa(ticks / 100, main_scratch, 10);
    puts(main_scratch);
    puts(" seconds\n");
}

void clear()
{
    vga_init(colour);
}

void setcolour()
{
    puts("fg: ");
    gets(main_scratch);
    uint8_t fg = colour_from_str(main_scratch);
    puts("bg: ");
    gets(main_scratch);
    uint8_t bg = colour_from_str(main_scratch);

    colour = vga_colour(fg, bg);
    vga_init(colour);
}

void clock()
{
    while (1)
    {
        if (keyboard_available())
        {
            if (keyboard_getchar(0) == 'q')
                break;
        }

        if (ticks % 100 == 0)
        {
            itoa(ticks / 100, main_scratch, 10);
            puts("\r");
            puts(main_scratch);
        }
        hlt();
    }
    puts("\n");
}

command_t commands[] = {
    {"uptime", uptime},
    {"clear", clear},
    {"clock", clock},
    {"setcolour", setcolour}
};


void set_timer_reload(uint16_t reload)
{
    outb(0x40, reload & 0xff);
    outb(0x40, reload >> 8);
}

void handle_irq0(uint32_t int_no, uint32_t err_no)
{
    ticks++;
}

void main()
{
    // Set to roughly 100hz
    set_timer_reload(11932);

    ticks = 0;
    register_handler(IRQ_TO_INTR(0), handle_irq0);

    colour = vga_get_default();

    char cmdbuf[64];

    while (1)
    {
        puts(prompt);
        gets(cmdbuf);
        
        if (*cmdbuf)
        {
            int found = 0;
            for (int i = 0; i < sizeof(commands) / sizeof(command_t); i++)
            {
                if (strcmp(commands[i].name, cmdbuf) == 0)
                {
                    commands[i].fn();
                    found = 1;
                    break;
                }
            }

            if (!found)
            {
                puts("? ");
                puts(cmdbuf);
                puts("\n");
            }
        }
    }
}