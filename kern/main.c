#include "stdlib.h"
#include "stdint.h"
#include "sys/interrupts.h"
#include "io/framebuffer.h"
#include "io/console.h"
#include "io/fbcon.h"
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
#include "selftest.h"
#include "config.h"
#include "io/conlib.h"

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

void setscheme(int argc, char** argv)
{
    uint32_t colours[] = {
        0x1d2021,
        0xfb4934,
        0xb8bb26,
        0xfabd2f,
        0x83a598,
        0xd3869b,
        0x8ec07c,
        0xd5c4a1,
        0x665c54,
        0xfb4934,
        0xb8bb26,
        0xfabd2f,
        0x83a598,
        0xd3869b,
        0x8ec07c,
        0xfbf1c7,
    };

    struct device* fbcon = device_get_by_name("fbcon0");
    fbcon->setparam(fbcon, FBCON_SETPARAM_COLOURSCHEME, &colours);
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
    puts("read        - print out the contents of a file or directory\n");
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

void pwd(int argc, char** argv)
{
    printf("%s\n", current_dir);
}

void read(int argc, char** argv)
{
    if (argc != 2) {
        printf("Usage: %s file_name\n", argv[0]);
        return;
    }

    filehandle_t* file = fs_open(argv[1]);

    if (file) {
        char buf[513];
        memset(buf, 0, 513);
        int rc;
        while ((rc = fs_read(file, buf, 512)) > 0) {
            printf("%s", buf);
            memset(buf, 0, 513);
        }
        fs_close(file);
    } else {
        printf("No such file or directory\n");
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

    filehandle_t* handle = fs_open(argv[1]);
    if (!handle) {
        printf("No such file\n");
        return;
    }

    size_t size;
    void* module = fs_read_full(handle, &size);

    mod_load(module);

    fs_close(handle);
    kfree(module);
}

/*
void fexec(fs_file_t file, int argc, char** argv)
{
    uint32_t fsize = fs_fsize(file);
    char* c = kallocz(fsize + 1);
    fs_fread(file, 0, fsize, c);

    elf_run(c, argc, argv);

    kfree(c);
    fs_fdestroy(file);
}
*/

void exec(int argc, char** argv)
{
    if (argc != 2) {
        printf("Usage: %s file_name\n", argv[0]);
        return;
    }

    /*
    fs_file_t file = fs_open(argv[1]);
    if (file != FS_FILE_INVALID) {
        fexec(file, argc, argv);
    } else {
        printf("No such file: %s\n", argv[1]);
    }
    */
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

void setconf(int argc, char** argv)
{
    if (argc < 2) {
        printf("Usage: %s namespace:key \"value\"\n", argv[0]);
        return;
    }

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

    config_setstr(argv[1], buf + 1);
    kfree(buf); // string is duplicated so we're safe to free this
}

void getconf(int argc, char** argv)
{
    if (argc < 1) {
        printf("Usage: %s namespace:key\n", argv[0]);
        return;
    }

    int type = config_gettype(argv[1]);
    switch (type) {
    case 0:
        printf("Key does not exist\n");
        break;
    case CONFIG_TYPE_INT:
        int ival = config_getint(argv[1]);
        printf("INT     %s = %d (hex %x)\n", argv[1], ival, ival);
        break;
    case CONFIG_TYPE_STR:
        const char* sval = config_getstr(argv[1]);
        printf("STRING  %s = \"%s\"\n", argv[1], sval);
        break;
    case CONFIG_TYPE_OBJ:
        void* oval = config_getobj(argv[1]);
        printf("OBJECT  %s = %p\n", argv[1], oval);
        break;
    default:
        printf("Unknown type %d", type);
        break;
    }
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

void display_sysinfo()
{
    char infobuf[255];
    snprintf(infobuf, 255,
        "SYSTEM SUMMARY " VER_NAME "\n\n"
        "branch        " VER_GIT_BRANCH "\n"
        "commit-id     " VER_GIT_REV "\n"
        "build-date    " VER_BUILD_DATE "\n"
    );

    cl_box('*', '*', '*', 80, infobuf, CL_NONE);
}


void logo(int argc, char** argv)
{
    display_logo();
}

void sysinfo(int argc, char** argv)
{
    display_sysinfo();
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
    {"read", read},
    {"pwd", pwd},
    {"echo", echo},
    {"setenv", setenv},
    {"setconf", setconf},
    {"getconf", getconf},
    {"poweroff", poweroff},
    {"exit", poweroff},
    {"help", help},
    {"logo", logo},
    {"sysinfo", sysinfo},
    {"selftest", selftest},
    {"setscheme", setscheme},
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

    /*
    fs_file_t file = fs_open(bin_name);
    if (file != FS_FILE_INVALID) {
        fexec(file, argc, argv);
        return 1;
    } else {
        return 0;
    }
    */
}

void main()
{
    char cmdbuf[64];

    display_logo();
    display_sysinfo();

    while (1) {
        puts(config_getstrns("sys", "prompt"));
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
