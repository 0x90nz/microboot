#include <stddef.h>
#include "syscalls.h"
#include "stdlib.h"
#include "mod.h"

int syscall_puts(uint32_t* args)
{
    ASSERT(*args, "Null call to puts");
    puts((const char*)*args);
    return 0;
}

int syscall_sysgetdynamic(uint32_t* args)
{
    return get_syscall_dynamic((const char*)*args);
}

int syscall_getmodsym(uint32_t* args)
{
    return (int)mod_sym_get((const char*)*args);
}

void syscalls_init()
{
    register_syscall_static("sysgetdynamic", syscall_sysgetdynamic, 0);
    REGISTER_SYSCALL(puts);
    REGISTER_SYSCALL(getmodsym);
}
