#include "stdlib.h"
#include "stdint.h"
#include "sys/interrupts.h"
#include "io/console.h"
#include "io/pio.h"
#include "io/serial.h"
#include "io/keyboard.h"
#include "io/ioutil.h"
#include "fs/fs.h"
#include "kernel.h"
#include "env.h"
#include "stddef.h"
#include "alloc.h"
#include "printf.h"
#include "sys/cpuid.h"
#include "exe/elf.h"
#include "mod.h"

int ticks;
char main_scratch[64];
char current_dir[256];

typedef struct {
    const char* name;
    void (*fn)(int, char**);
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
    for (int i = 0; i < sizeof(colours) / sizeof(const char*); i++) {
        if (strcmp(colours[i], str) == 0)
            return i;
    }
    return 0;
}

void uptime(int argc, char** argv)
{
    printf("Up approximately %d.%d seconds\n", ticks / 100, ticks % 100);
}

void clear(int argc, char** argv)
{
    console_clear(stdout);
}

void setcolour(int argc, char** argv)
{
    if (argc != 3) {
        printf("Usage: %s fg_colour bg_colour\n", argv[0]);
        return;
    }

    uint8_t fg = colour_from_str(argv[1]);
    uint8_t bg = colour_from_str(argv[2]);

    console_colour(stdout, fg | bg << 4);
    console_clear(stdout);
}

void listcolours(int argc, char** argv)
{
    for (int i = 0; i < sizeof(colours) / sizeof(colours[0]); i++) {
        printf("%s\n", colours[i]);
    }
}

void clock(int argc, char** argv)
{
    while (1) {
        if (keyboard_available()) {
            if (keyboard_getchar(0) == 'q')
                break;
        }

        printf("\r%d.%d", ticks / 100, ticks % 100);
        hlt();
    }
    puts("\n");
}

void dino(int argc, char** argv)
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

void help(int argc, char** argv)
{
    puts("uptime      - display uptime in seconds\n");
    puts("mem         - get memory status\n");
    puts("clear       - clear the display\n");
    puts("clock       - updating clock demo\n");
    puts("setcolour   - set the display colour\n");
    puts("listcolours - list all available colours\n");
    puts("scancode    - display raw scancodes\n");
    puts("verb        - set log verbosity\n");
    puts("hdisk       - show the root disk number\n");
    puts("ls          - list the current directory [WIP]\n");
    puts("pwd         - print the working directory [WIP]\n");
    puts("cat         - print out the contents of a file [WIP]\n");
    puts("poweroff    - shut down the computer\n");
    puts("exit        - alias to poweroff\n");
    puts("help        - this help message\n");
}

void scancode(int argc, char** argv)
{
    puts("esc to exit\n");
    while (1) {
        uint8_t code = keyboard_poll_scancode();
        itoa(code, main_scratch, 10);
        puts(main_scratch);
        puts("\n");
        if (code == 1)
            break;
    }
}

void verb(int argc, char** argv)
{
    if (argc != 2) {
        printf("Usage: %s log_level\n", argv[0]);
        return;
    }

    int level = atoi(argv[1]);
    set_log_level(level);
}

void brk(int argc, char** argv)
{
	asm("int $3");
}

void hdisk(int argc, char** argv)
{
    printf("Root disk @ %02x\n", *env_get(get_rootenv(), "root", uint8_t*));
}

void ls(int argc, char** argv)
{
    fs_t* fs = env_get(get_rootenv(), "rootfs", fs_t*);
    if (fs) {
        fs_file_t dir = fs_traverse(fs, argc > 1 ? argv[1] : "");
        if (dir)
            fs_list_dir(fs, dir);
        fs_destroy(fs, dir);
    }
}

void pwd(int argc, char** argv)
{
    printf("%s\n", current_dir);
}

void cat(int argc, char** argv)
{
    if (argc != 2) {
        printf("Usage: %s file_name\n", argv[0]);
        return;
    }

    fs_t* fs = env_get(get_rootenv(), "rootfs", fs_t*);
    if (fs) {
        fs_file_t file = fs_traverse(fs, argv[1]);
        if (file) {
            uint32_t fsize = fs_fsize(fs, file);
            char* c = kallocz(fsize + 1);
            fs_read(fs, file, 0, fsize, c);
            printf("%s\n", c);
            kfree(c);
            fs_destroy(fs, file);
        } else {
            printf("No such file: %s\n", argv[1]);
        }
    }
}

void echo(int argc, char** argv)
{
    for (int i = 1; i < argc - 1; i++) {
        printf("%s ", argv[i]);
    }
    printf("%s\n", argv[argc - 1]);
}

void poweroff(int argc, char** argv)
{
    kpoweroff();
}

void mem(int argc, char** argv)
{
    size_t used = alloc_used(0);
    size_t total = alloc_total();
    printf("%d bytes / %d KiB used\n", used, total / KiB);
}

void ldmod(int argc, char** argv)
{
    if (argc != 2) {
        printf("Usage: %s file_name\n", argv[0]);
        return;
    }

    fs_t* fs = env_get(get_rootenv(), "rootfs", fs_t*);
    if (fs) {
        fs_file_t file = fs_traverse(fs, argv[1]);
        if (file) {
            uint32_t fsize = fs_fsize(fs, file);
            char* c = kallocz(fsize + 1);
            fs_read(fs, file, 0, fsize, c);
            
            mod_load(c);

            kfree(c);
            fs_destroy(fs, file);
        } else {
            printf("No such file: %s\n", argv[1]);
        }
    }
}

void exec(int argc, char** argv)
{
    if (argc != 2) {
        printf("Usage: %s file_name\n", argv[0]);
        return;
    }

    fs_t* fs = env_get(get_rootenv(), "rootfs", fs_t*);
    if (fs) {
        fs_file_t file = fs_traverse(fs, argv[1]);
        if (file) {
            uint32_t fsize = fs_fsize(fs, file);
            char* c = kallocz(fsize + 1);
            fs_read(fs, file, 0, fsize, c);
            
            elf_run(c, argc, argv);

            kfree(c);
            fs_destroy(fs, file);
        } else {
            printf("No such file: %s\n", argv[1]);
        }
    }
}

void lsmod(int argc, char** argv)
{
    mod_list();
}

void lssym(int argc, char** argv)
{
    mod_sym_list();
}


void cmd_cpuid(int argc, char** argv)
{
    char buf[13];
    cpuid_get_vendor(buf);
    
    printf("vendor         %s\n", buf);
    printf("max_id         %d\n", cpuid_get_max_id());
    cpuid_verbose_dump();
}

command_t commands[] = {
    {"exec", exec},
    {"lsmod", lsmod},
    {"ldmod", ldmod},
    {"lssym", lssym},
    {"uptime", uptime},
    {"mem", mem},
    {"cpuid", cmd_cpuid},
    {"clear", clear},
    {"clock", clock},
    {"setcolour", setcolour},
    {"listcolours", listcolours},
    {"dino", dino},
    {"scancode", scancode},
    {"verb", verb},
    {"brk", brk},
    {"hdisk", hdisk},
    {"ls", ls},
    {"cat", cat},
    {"pwd", pwd},
    {"echo", echo},
    {"poweroff", poweroff},
    {"exit", poweroff},
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

void main()
{
    // Set to roughly 100hz
    set_timer_reload(11932);

    ticks = 0;
    register_handler(IRQ_TO_INTR(0), handle_irq0);

    char cmdbuf[64];

    while (1) {
        puts(env_get(get_rootenv(), "prompt", const char*));
        gets(cmdbuf);
        
        if (*cmdbuf) {
            int argc = 1;
            char* token = strtok(cmdbuf, " ");
            while (strtok(NULL, " ") != NULL) { argc++; }

            char** argv = kalloc(sizeof(char*) * argc);

            char* tmp = token;
            for (int i = 0; i < argc; i++) {
                argv[i] = tmp;
                tmp += strlen(tmp) + 1;
            }

            int found = 0;
            for (int i = 0; i < sizeof(commands) / sizeof(command_t); i++) {
                if (strcmp(commands[i].name, token) == 0) {
                    commands[i].fn(argc, argv);
                    found = 1;
                    break;
                }
            }

            if (!found) {
                printf("? %s\n", cmdbuf);
            }

            kfree(argv);
        }
    }
}
