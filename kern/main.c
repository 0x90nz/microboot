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
}

void setfnt(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: %s font.fnt\n", argv[0]);
        return;
    }

    struct device* dev = device_get_by_name("fbcon0");
    if (!dev)
        return;

    filehandle_t* file = fs_open(argv[1]);
    struct fbcon_font* font = kalloc(sizeof(*font));

    if (file) {
        size_t size;
        void* data = fs_read_full(file, &size);
        if (!data || size == 0) {
            printf("Unable to read font\n");
            fs_close(file);
            return;
        }

        debugf("read %d, data@%p", size, data);
        font->char_width = *(uint8_t*)data;
        font->char_height = *(uint8_t*)(data + 1);
        font->char_width_bytes = font->char_width / 8 + (font->char_width % 8 != 0);
        font->data = data + 4; // skip over header
        dev->setparam(dev, FBCON_SETPARAM_FONT, font);

        // for some reason fs close is borked
        fs_close(file);
    } else {
        printf("No such file or directory\n");
    }
}

void setscheme(int argc, char** argv)
{
    if (argc != 17) {
        printf("Usage: %s c1 c2 c3 c4 c5 c6 c7 c8 c9 c10 c11 c12 c13 c14 c15 c16", argv[0]);
        printf("       where c1-c16 are the 16 colours in order set out by `listcolours`\n");
    }

    uint32_t colours[16];
    for (int i = 0; i < 16; i++) {
        colours[i] = strtoul(argv[i + 1], NULL, 16);
    }
    struct device* fbcon = device_get_by_name("fbcon0");
    if (fbcon) {
        fbcon->setparam(fbcon, FBCON_SETPARAM_COLOURSCHEME, &colours);
    }
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

void dumpmem(int argc, char** argv)
{
    if (argc != 3) {
        printf("Usage: %s addr len\n", argv[0]);
        printf("       addr and len must be in hex, with no '0x' prefix.\n");
        return;
    }

    uint32_t addr = strtoul(argv[1], NULL, 16);
    uint32_t len = strtoul(argv[2], NULL, 16);
    dump_memory((void*)addr, len);
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

void exec(int argc, char** argv)
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

    elf_run(module, argc - 1, argv + 1);

    fs_close(handle);
    kfree(module);
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
    printf("%-32s %-8s %p\n", drv->name, drv->modname, drv);
}

void lsdrv(int argc, char** argv)
{
    printf("%-32s %-8s %s\n", "name", "modname", "ptr");
    driver_foreach(lsdrv_callback);
}

void endrv(int argc, char** argv)
{
    if (argc != 2) {
        printf("Usage: %s driver_modname\n", argv[0]);
        return;
    }

    struct driver* driver = driver_get_by_modname(argv[1]);
    if (!driver) {
        printf("No such driver '%s'\n", argv[1]);
        return;
    }

    if (!driver->disabled) {
        printf("Driver '%s' already enabled\n", argv[1]);
    }

    driver_enable(driver);
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
    {"endrv", endrv},
    {"uptime", uptime},
    {"mem", mem},
    {"ver", ver},
    {"cpuid", cmd_cpuid},
    {"clear", clear},
    {"clock", clock},
    {"setres", setres},
    {"setfnt", setfnt},
    {"setcolour", setcolour},
    {"listcolours", listcolours},
    {"dino", dino},
    {"scancode", scancode},
    {"verb", verb},
    {"brk", brk},
    {"dumpmem", dumpmem},
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

    filehandle_t* handle = fs_open(bin_name);

    if (!handle) {
        return 0;
    }

    size_t size;
    void* module = fs_read_full(handle, &size);
    debugf("@%p, %d", module, size);

    elf_run(module, argc, argv);

    fs_close(handle);
    kfree(module);
    return 1;
}

void process_command_string(char* cmdbuf)
{
    // if there are any comments, the line ends immediately after them.
    size_t cmdbuf_len = strlen(cmdbuf);
    for (size_t i = 0; i < cmdbuf_len; i++) {
        if (cmdbuf[i] == '#') {
            cmdbuf[i] = '\0';
            break;
        }
    }

    // it's possible the entire line could be a comment, so just ignore
    // it if so.
    if (cmdbuf[0] == '\0')
        return;

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

void process_autorun()
{
    filehandle_t* file = fs_open("autorun.esh");

    if (file) {
        size_t size;
        void* data = fs_read_full(file, &size);
        if (!data || size == 0) {
            printf("Unable to read autorun\n");
            fs_close(file);
            return;
        }
        data = krealloc(data, size + 1);
        ((uint8_t*)data)[size] = '\0';

        // NOTE: reentrant strtok is ESSENTIAL! we use strtok within a function called
        // from here, so w/out it we'd clobber the current state.
        char* saveptr;
        char* tok = strtok_r(data, "\n", &saveptr);
        while (tok) {
            // NOTE: we need to duplicate cmd here because processing the
            // cmd string modifies it, and that would mess with strtok.
            char* cmd = strdup(tok);
            debugf("cmd: \"%s\"", tok);
            process_command_string(cmd);
            kfree(cmd);

            tok = strtok_r(NULL, "\n", &saveptr);
        }

        kfree(data);
    } else {
        debug("autorun not present");
    }
}

void main()
{
    char cmdbuf[256];

    process_autorun();

    while (1) {
        puts(config_getstrns("sys", "prompt"));
        gets(cmdbuf);

        if (*cmdbuf) {
            process_command_string(cmdbuf);
        }
    }
}
