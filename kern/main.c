#include "stdlib.h"
#include "stdint.h"
#include "sys/interrupts.h"
#include "io/framebuffer.h"
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
#include "version.h"

char main_scratch[64];
char current_dir[256];

struct command {
    const char* name;
    void (*fn)(int, char**);
};

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
    uint32_t ticks = kticks();
    printf("Up approximately %d.%d seconds\n", ticks / 100, ticks % 100);
}

void clear(int argc, char** argv)
{
    console_clear(console);
}

void setcolour(int argc, char** argv)
{
    if (argc != 3) {
        printf("Usage: %s fg_colour bg_colour\n", argv[0]);
        return;
    }

    uint8_t fg = colour_from_str(argv[1]);
    uint8_t bg = colour_from_str(argv[2]);

    console_colour(console, fg | bg << 4);
    console_clear(console);
}

void setres(int argc, char** argv)
{
    if (argc != 4) {
        printf("Usage: %s xres yres bpp\n", argv[0]);
        return;
    }

    int xres = atoi(argv[1]);
    int yres = atoi(argv[2]);
    int bpp = atoi(argv[3]);

    struct device* dev = device_get_by_name("vesafb0"); // TODO: get properly
    if (!dev)
        return;

    struct fbdev_setres_request req = {
        .xres = xres,
        .yres = yres,
        .bpp = bpp
    };
    dev->setparam(dev, FBDEV_SETRES, &req);

    struct device* vga = device_get_by_name("vga0"); // TODO: get properly
    if (vga)
        device_deregister(vga);

    struct device* fbcon = device_get_by_name("fbcon0");
    if (!fbcon)
        return;

    debugf("fbcon: %p", fbcon);
    console = device_get_console(fbcon); // TODO: get properly
    kfree(stdout);
    stdout = kalloc(sizeof(*stdout));
    console_get_chardev(console, stdout);
}

void listcolours(int argc, char** argv)
{
    for (int i = 0; i < sizeof(colours) / sizeof(colours[0]); i++) {
        printf("%s\n", colours[i]);
    }
}

void clock(int argc, char** argv)
{
    puts("Clock demo. 'q' to exit\n");
    while (1) {
        if (keyboard_available()) {
            if (keyboard_getchar(0) == 'q')
                break;
        }

        uint32_t ticks = kticks();
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
    puts("Help Summary. WIP commands marked with [WIP] or if very unstable, not listed.\n");
    puts("uptime      - display uptime in seconds\n");
    puts("mem         - get memory status\n");
    puts("cpuid       - display CPU info\n");
    puts("brk         - cause a #BP interrupt\n");
    puts("clear       - clear the display\n");
    puts("clock       - updating clock demo\n");
    puts("setcolour   - set the display colour\n");
    puts("listcolours - list all available colours\n");
    puts("scancode    - display raw scancodes\n");
    puts("verb        - set log verbosity\n");
    puts("ls          - list the current directory [WIP]\n");
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

void ls(int argc, char** argv)
{
    fs_file_t dir = fs_open(argc > 1 ? argv[1] : "");
    if (dir != FS_FILE_INVALID)
        fs_flist(dir);
    else
        printf("Invalid directory\n");
    fs_fdestroy(dir);
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

    fs_file_t file = fs_open(argv[1]);
    if (file != FS_FILE_INVALID) {
        uint32_t fsize = fs_fsize(file);
        char* c = kallocz(fsize + 1);
        fs_fread(file, 0, fsize, c);
        printf("%s\n", c);
        kfree(c);
        fs_fdestroy(file);
    } else {
        printf("No such file: %s\n", argv[1]);
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
    size_t allocated = alloc_used(1);
    size_t total = alloc_total();

    printf("Memory status:\n");
    printf("Used      = %d bytes\n", used);
    printf("Allocated = %d bytes (includes freed)\n", allocated);
    printf("Available = %d bytes\n", total - used); 
    printf("Total     = %d bytes\n", total);
}

void ldmod(int argc, char** argv)
{
    if (argc != 2) {
        printf("Usage: %s file_name\n", argv[0]);
        return;
    }

    fs_file_t file = fs_open(argv[1]);
    if (file != FS_FILE_INVALID) {
        uint32_t fsize = fs_fsize(file);
        char* c = kallocz(fsize + 1);
        fs_fread(file, 0, fsize, c);

        mod_load(c);

        kfree(c);
        fs_fdestroy(file);
    } else {
        printf("No such file: %s\n", argv[1]);
    }
}

void fexec(fs_file_t file, int argc, char** argv)
{
    uint32_t fsize = fs_fsize(file);
    char* c = kallocz(fsize + 1);
    fs_fread(file, 0, fsize, c);

    elf_run(c, argc, argv);

    kfree(c);
    fs_fdestroy(file);
}

void exec(int argc, char** argv)
{
    if (argc != 2) {
        printf("Usage: %s file_name\n", argv[0]);
        return;
    }

    fs_file_t file = fs_open(argv[1]);
    if (file != FS_FILE_INVALID) {
        fexec(file, argc, argv);
    } else {
        printf("No such file: %s\n", argv[1]);
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

void setenv(int argc, char** argv)
{
    if (argc < 2) {
        printf("Usage: %s var \"value\"\n", argv[0]);
        return;
    }

    // XXX: this causes a memory leak on overwrite
    size_t bufsz = 0;
    for (int i = 2; i < argc; i++) {
        bufsz += strlen(argv[i]) + 1;
    }

    char* buf = kallocz(bufsz);
    for (int i = 2; i < argc - 1; i++) {
        strcat(buf, argv[i]);
        strcat(buf, " ");
    }
    strcat(buf, argc[argv - 1]);

    if (buf[0] != '"') {
        printf("value must start with quote\n");
        kfree(buf);
        return;
    }

    if (buf[bufsz - 2] != '"') {
        printf("value must end with quote\n");
        kfree(buf);
        return;
    }

    buf[bufsz - 2] = '\0';
    buf++;

    env_put(get_rootenv(), argv[1], buf);
}

void ver(int argc, char** argv)
{
    printf("%s - %s (built %s)\n", VER_NAME, VER_GIT_REV, VER_BUILD_DATE);
}

void lsdev_callback(struct device* dev)
{
    printf("%s\n", dev->name);
}

void lsdev(int argc, char** argv)
{
    device_foreach(lsdev_callback);
}

void lsdrv_callback(struct driver* drv)
{
    printf("%s\n", drv->name);
}

void lsdrv(int argc, char** argv)
{
    driver_foreach(lsdrv_callback);
}

void display_logo()
{
    printf("\n");
    printf("%-16s           ##                            ##\n", "");
    printf("%-16s           ##                            ##\n", "");
    printf("%-16s ##    ##  ########   #######   ####### #####\n", "");
    printf("%-16s ##    ##  ##    ### ###   ### ###   ### ##\n", "");
    printf("%-16s ##    ##  ##     ## ##     ## ##     ## ##\n", "");
    printf("%-16s ##    ##  ###   ### ###   ### ###   ### ##\n", "");
    printf("%-16s # ####  # ########   #######   #######  #####\n", "");
    printf("%-16s ##\n", "");
    printf("%-16s ##\n", "");
}

void logo(int argc, char** argv)
{
    display_logo();
}

static struct command commands[] = {
    {"exec", exec},
    {"lsmod", lsmod},
    {"ldmod", ldmod},
    {"lssym", lssym},
    {"lsdev", lsdev},
    {"lsdrv", lsdrv},
    {"uptime", uptime},
    {"mem", mem},
    {"ver", ver},
    {"cpuid", cmd_cpuid},
    {"clear", clear},
    {"clock", clock},
    {"setres", setres},
    {"setcolour", setcolour},
    {"listcolours", listcolours},
    {"dino", dino},
    {"scancode", scancode},
    {"verb", verb},
    {"brk", brk},
    {"ls", ls},
    {"cat", cat},
    {"pwd", pwd},
    {"echo", echo},
    {"setenv", setenv},
    {"poweroff", poweroff},
    {"exit", poweroff},
    {"help", help},
    {"logo", logo},
};

// Invoke a shell-internal function. Returns a non-zero value if a function
// was invoked; zero otherwise.
int invoke_internal(const char* name, int argc, char** argv)
{
    for (int i = 0; i < sizeof(commands) / sizeof(struct command); i++) {
        if (strcmp(commands[i].name, name) == 0) {
            commands[i].fn(argc, argv);
            return 1;
        }
    }
    return 0;
}

// Invoke a shell-external program. Returns a non-zero value if the program
// was invoked; zero otherwise.
int invoke_external(const char* name, int argc, char** argv)
{
    char bin_name[256];
    strcpy(bin_name, "bin/");
    strcat(bin_name, name);
    strcat(bin_name, ".elf");

    fs_file_t file = fs_open(bin_name);
    if (file != FS_FILE_INVALID) {
        fexec(file, argc, argv);
        return 1;
    } else {
        return 0;
    }
}

void main()
{
    char cmdbuf[64];

    display_logo();

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

            if (
                !invoke_internal(token, argc, argv)
                && !invoke_external(token, argc, argv)
            ) {
                printf("? %s\n", cmdbuf);
            }

            kfree(argv);
        }
    }
}
