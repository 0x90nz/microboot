#include "io/driver.h"
#include "io/pio.h"
#include "io/vga.h"
#include "io/vesa.h"
#include "io/fbcon.h"
#include "io/pci.h"
#include "io/serial.h"
#include "io/console.h"
#include "io/keyboard.h"
#include "fs/fs.h"
#include "fs/envfs.h"
#include "sys/interrupts.h"
#include "sys/gdt.h"
#include "sys/bios.h"
#include "sys/syscall.h"
#include "syscalls.h"
#include "stdlib.h"
#include "kernel.h"
#include "alloc.h"
#include "env.h"
#include "list.h"
#include "mod.h"

char* debug_names[] = {
    "FATAL",
    "ERROR",
    "WARN",
    "INFO",
    "DEBUG",
    "ALL",
    "OFF"
};

// Startup information
static struct kstart_info sinfo;

// The root environment
static env_t* env;

// Number of ticks of the system clock
static uint32_t ticks;

// This is globally visible. Consider changing this to a special file handle?
chardev_t* stdout;
chardev_t* stdin;
chardev_t* dbgout;
console_t* console;


/**
 * @brief Hang the system forever. Useful to prevent the system from executing
 * random code in an error condition
 */
void hang() { while (1) { asm("hlt"); } }

/**
 * @brief Execute the asm 'hlt' instruction once
 */
void hlt() { asm("hlt"); }

/**
 * @brief Block until some indeterminate point in the future. Currently does
 * exactly the same thing as hlt, but not guaranteed to do so in the future.
 */
void yield() { hlt(); }

/**
 * @brief Get the current value of the esp (stack pointer) register
 *
 * @return uint32_t the value of the esp registers
 */
uint32_t get_esp()
{
    uint32_t esp;
    asm("mov    %%esp, %0" : "=m" (esp));
    return esp;
}

/**
 * @brief Dump a buffer of memory to output
 * 
 * @param input_buffer the buffer to dump
 * @param length the length to dump in bytes
 */
void dump_memory(void* input_buffer, size_t length)
{
    uint8_t* startaddr = (uint8_t*)input_buffer;
    uint8_t buffer[8];

    for(size_t i = 0; i < length; i += 8) {
        printf("0x%08x: ", (int)(startaddr + i));
        for(int j = 0; j < 8; j++) {
            buffer[j] = *(uint8_t*)(startaddr + i + j);
			printf("%02x ", buffer[j]);
            if((j + 1) % 4 == 0)
                printf(" ");
        }
        printf("   |");
        char charbuf[2];
        charbuf[1] = '\0';
        for(int j = 0; j < 8; j++) {
            if(buffer[j] >= 32 && buffer[j] <= 126) {
                charbuf[0] = buffer[j];
            } else {
                charbuf[0] = '.';
            }
            
            printf(charbuf);
        }
        printf("|\n");
    }
    printf("\n");
}

/**
 * @brief Power off the computer using the BIOS APM interface
 * 
 */
void kpoweroff()
{
    printf("shutting down!\n");
    debug("shutting down!\n");

    struct int_regs* regs = kallocz(sizeof(*regs));
    
    // Connect BIOS APM
    regs->eax = 0x5301;
    bios_interrupt(0x15, regs);

    ASSERT(!(regs->flags & EFL_CF), "APM not supported");
    
    // Connect to interface
    regs->eax = 0x530f;
    regs->ebx = 1;
    regs->ecx = 1;
    bios_interrupt(0x15, regs);

    // Execute command (shutdown)
    regs->eax = 0x5307;
    regs->ebx = 1;
    regs->ecx = 3;
    bios_interrupt(0x15, regs);
}

/**
 * @brief Get the number of system ticks.
 *
 * A system tick is defined as one invocation of the main system timer, IRQ0,
 * since boot. By default the rate is around 100Hz
 *
 * @return uint64_t the number of ticks
 */
uint32_t kticks()
{
    return ticks;
}

env_t* get_rootenv()
{
    return env;
}

// Why not just use __attribute__((naked))? Because it's not
// supported on older versions of GCC for the x86 arch
void switch_stacks(void* new);

asm(
    ".global switch_stacks\n\t"
    "switch_stacks:\n\t"
    "mov    4(%esp), %esp\n\t"
    "jmp    kernel_late_init"
);

void it_handle(int i, struct list_node* item)
{
    printf("%d: %s\n", i, item->value);
}

// Sets the timer reload value for IRQ0
static void set_timer_reload(uint16_t reload)
{
    outb(0x40, reload & 0xff);
    outb(0x40, reload >> 8);
}

static void irq0_handle(uint32_t int_no, uint32_t err_no)
{
    ticks++;
}

static void read_config()
{
    fs_file_t file = fs_open("config.txt");
    if (file == FS_FILE_INVALID)
        return;

    size_t size = fs_fsize(file);
    char* file_contents = kallocz(size + 1);
    fs_fread(file, 0, size, file_contents);

    env_kvp_lines_add(env, file_contents);

    fs_fdestroy(file);
}

void kernel_late_init()
{
    log(LOG_INFO, "early init complete");
    syscalls_init();

    // Set to roughly 100hz
    set_timer_reload(11932);
    ticks = 0;
    register_handler(IRQ_TO_INTR(0), irq0_handle);
    debug("timer initialised");

    env = env_init();
    env_put(env, "prompt", "# ");

    fs_init(sinfo.drive_number);
    fs_mount("sys", envfs_init(env));
    debug("fs initialised");

    read_config();
    debug("read config");

    debug("all init done. transferring to main");

    extern int main();
    main();

    hang();
}

extern int _kexp_start, _kexp_end;
void kernel_main(struct kstart_info* start_info)
{
    stdout = NULL;
    dbgout = NULL;
    console = NULL;

    gdt_init();
    init_alloc(start_info->memory_start, start_info->free_memory * 64 * KiB);

    driver_init();
    mod_init();

    void* ksymtab = &_kexp_start;
    size_t ksymtab_size = (void*)&_kexp_end - (void*)&_kexp_start;
    mod_ksymtab_early_init(ksymtab, ksymtab_size);

    dbgout = device_get_chardev(device_get_by_name("sp0")); // TODO: get first avail chardev?
    debug("debug serial up");

    mod_ksymtab_add(ksymtab, ksymtab_size);

    debug("module system initialised");

    interrupts_init();
    debug("interrupts initialised");

    // setup a VGA console for early init. will be replaced later if we want
    console = device_get_console(device_get_by_name("vga0"));
    stdout = kalloc(sizeof(*stdout));
    console_get_chardev(console, stdout);
    debug("early vga console initialised");

    syscall_init();
    debug("syscalls initialised");

    keyboard_init();
    stdin = kalloc(sizeof(*stdin));
    keyboard_get_chardev(stdin);
    debug("keyboard initialised");

    // Save start info because when we switch stacks it'll get destroyed
    memcpy(&sinfo, start_info, sizeof(struct kstart_info));

    /**
     * Setup and switch to a new stack. After we switch to this, we won't be
     * using any low memory apart from the BIOS interrupt code. This means that
     * we can (mostly) dedicate it to user programs
     */
    debug("switching stacks");
    size_t stack_size = 32 * KiB;
    void* stack = kalloc(stack_size);
    *(uint32_t*)stack = STACK_MAGIC;
    switch_stacks(stack + stack_size);

    // We shouldn't ever get here
    hang();
}