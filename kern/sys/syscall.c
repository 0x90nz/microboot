#include "syscall.h"
#include "interrupts.h"
#include "../stdlib.h"

static struct syscall_entry syscall_table[SYSCALL_NR_MAX];
static int syscall_max = SYSCALL_NR_RESERVED;

void syscall_handle(intr_frame_t* frame)
{
    unsigned int syscall_nr = frame->eax;
    
    if (syscall_nr >= 0 && syscall_nr < SYSCALL_NR_MAX) {
        if (syscall_table[syscall_nr].fn) {
            uint32_t args[SYSCALL_NR_ARGS];
            args[0] = frame->ecx;
            args[1] = frame->edx;
            args[2] = frame->ebx;
            args[3] = frame->esi;
            args[4] = frame->edi;
            frame->eax = syscall_table[syscall_nr].fn(args);
        } else {
            debugf("spurious syscall %d", syscall_nr);
        }
    }
}

/**
 * Register a dynamic syscall. Returns a non-zero value on success
 * If successful and nr is non null, it will contain the syscall
 * number which was assigned
 */ 
int register_syscall(const char* name, syscall_fn_t func, int* nr)
{
    if (syscall_max + 1 < SYSCALL_NR_MAX) {
        if (nr)
            *nr = syscall_max;

        syscall_table[syscall_max].fn = func;
        syscall_table[syscall_max++].name = name;
        return 1;
    }
    return 0;
}

/**
 * Register a static (known-number) syscall. A non-zero return value
 * indicates success  
 */ 
int register_syscall_static(const char* name, syscall_fn_t func, int nr)
{
    if (nr >= 0 && nr < SYSCALL_NR_RESERVED) {
        syscall_table[nr].fn = func;
        syscall_table[nr].name = name;
        return 1;
    }
    return 0;
}

int get_syscall_dynamic(const char* name)
{
    for (int i = 0; i < syscall_max; i++) {
        if (syscall_table[i].fn && syscall_table[i].name) {
            if (strcmp(syscall_table[i].name, name) == 0)
                return i;
        }
    }
    return -1;
}

void syscall_init()
{
    for (int i = 0; i < SYSCALL_NR_RESERVED; i++) {
        syscall_table[i].fn = NULL;
        syscall_table[i].name = NULL;   
    }
    register_ll_handler(128, syscall_handle);
}