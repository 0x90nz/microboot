#include "io/pio.h"
#include "io/vga.h"
#include "io/pci.h"
#include "io/serial.h"
#include "io/keyboard.h"
#include "fs/fs.h"
#include "sys/interrupts.h"
#include "sys/gdt.h"
#include "sys/bios.h"
#include "sys/syscall.h"
#include "stdlib.h"
#include "kernel.h"
#include "alloc.h"
#include "env.h"

char* debug_names[] = {
    "FATAL",
    "ERROR",
    "WARN",
    "INFO",
    "DEBUG",
    "ALL",
    "OFF"
};

static struct kstart_info sinfo;
static env_t* env;

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

void display_logo()
{
    vga_puts("\n");
    int offset = 16;
    vga_pad(offset); vga_puts("           ##                            ##\n");
    vga_pad(offset); vga_puts("           ##                            ##\n");
    vga_pad(offset); vga_puts(" ##    ##  ########   #######   ####### #####\n");
    vga_pad(offset); vga_puts(" ##    ##  ##    ### ###   ### ###   ### ##\n");
    vga_pad(offset); vga_puts(" ##    ##  ##     ## ##     ## ##     ## ##\n");
    vga_pad(offset); vga_puts(" ##    ##  ###   ### ###   ### ###   ### ##\n");
    vga_pad(offset); vga_puts(" # ####  # ########   #######   #######  #####\n");
    vga_pad(offset); vga_puts(" ##\n");
    vga_pad(offset); vga_puts(" ##\n");
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

env_t* get_rootenv()
{
    return env;
}

__attribute__((naked)) void switch_stacks(void* new)
{
    asm("mov    4(%esp), %esp");
    asm("jmp    kernel_late_init");
}

void syscall_puts(uint32_t* args)
{
    puts((const char*)*args);   
}

void kernel_late_init()
{
    fs_t* fs = fs_init(sinfo.drive_number);
    env_put(env, "rootfs", fs);

    extern int main();
    main();

    hang();
}

void kernel_main(struct kstart_info* start_info)
{
    vga_init(vga_colour(VGA_WHITE, VGA_BLUE));
    init_alloc(start_info->memory_start, start_info->free_memory * 64 * KiB);

    interrupts_init();
    gdt_init();
    syscall_init();
    keyboard_init();
    serial_init(SP_COM0_PORT);
    display_logo();

    env = env_init();
    env_put(env, "prompt", "# ");
    env_put(env, "root", &start_info->drive_number);

    register_syscall_static("puts", syscall_puts, 0);

    // Save start info because when we switch stacks it'll get destroyed
    memcpy(&sinfo, start_info, sizeof(struct kstart_info));

    /**
     * Setup and switch to a new stack. After we switch to this, we won't be
     * using any low memory apart from the BIOS interrupt code. This means that
     * we can (mostly) dedicate it to user programs
     */ 
    size_t stack_size = 32 * KiB;
    void* stack = kalloc(stack_size);
    *(uint32_t*)stack = STACK_MAGIC;
    switch_stacks(stack + stack_size);

    // We shouldn't ever get here
    hang();
}