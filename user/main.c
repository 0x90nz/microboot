#include <stdlib.h>
#include <stdint.h>
#include <interrupts.h>
#include <vga.h>
#include <pio.h>
#include <kernel.h>
#include <keyboard.h>

#include <ne2k.h>
#include <ether.h>
#include <ip.h>
#include <stddef.h>
#include <alloc.h>

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
    "lgreen",
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

void dino()
{
    puts("                         .       .\n");
    puts("                        / `.   .' \\\n");
    puts("                .---.  <    > <    >  .---.\n");
    puts("                |    \\  \\ - ~ ~ - /  /    |\n");
    puts("                 ~-..-~             ~-..-~\n");
    puts("             \\~~~\\.'                    `./~~~/\n");
    puts("              \\__/                        \\__/\n");
    puts("               /                  .-    .  \\\n");
    puts("        _._ _.-    .-~ ~-.       /       }   \\/~~~/\n");
    puts("    _.-'q  }~     /       }     {        ;    \\__/\n");
    puts("   {'__,  /      (       /      {       /      `. ,~~|   .     .\n");
    puts("    `''''='~~-.__(      /_      |      /- _      `..-'   \\\\   //\n");
    puts("                / \\   =/  ~~--~~{    ./|    ~-.     `-..__\\\\_//_.-'\n");
    puts("               {   \\  +\\         \\  =\\ (        ~ - . _ _ _..---~\n");
    puts("               |  | {   }         \\   \\_\\\n");
    puts("              '---.o___,'       .o___,'\n");
}

void help()
{
    puts("uptime    - display uptime in seconds\n");
    puts("clear     - clear the display\n");
    puts("clock     - updating clock demo\n");
    puts("setcolour - set the display colour\n");
    puts("scancode  - display raw scancodes\n");
    puts("help      - this help message\n");
}

void scancode()
{
    puts("esc to exit\n");
    while (1)
    {
        uint8_t code = keyboard_poll_scancode();
        itoa(code, main_scratch, 10);
        puts(main_scratch);
        puts("\n");
        if (code == 1)
            break;
    }
}

command_t commands[] = {
    {"uptime", uptime},
    {"clear", clear},
    {"clock", clock},
    {"setcolour", setcolour},
    {"dino", dino},
    {"scancode", scancode},
    {"help", help}
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


void net_test()
{
    int size = 1024;
    size_t pktsz = ether_buffer_length(ip_buffer_length(size));
    
    uint8_t* packet = (uint8_t*)kalloc(pktsz);
    uint8_t src[] = {0xde, 0xad, 0xbe, 0xef, 0xc0, 0xfe};
    char* ether = ether_make_packet(packet, src, src, ip_buffer_length(size));
    char* ip = ip_make_packet(ether, size, 0x11, 0xffffffff, 0xffffffff);
    
    ne2k_tx_packet(packet, pktsz);
}

void main()
{
    // Set to roughly 100hz
    set_timer_reload(11932);

    ticks = 0;
    register_handler(IRQ_TO_INTR(0), handle_irq0);

    colour = vga_get_default();

    net_test();

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