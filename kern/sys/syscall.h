#pragma once

#include <stdint.h>

#define SYSCALL_NR_ARGS     5
#define SYSCALL_NR_MAX      128
#define SYSCALL_NR_RESERVED 32

typedef void (*syscall_fn_t)(uint32_t* args);

struct syscall_entry {
    const char* name;
    syscall_fn_t fn;
};

void syscall_init();
void do_syscall(uint32_t nr, int arg0, int arg1, int arg2, int arg3, int arg4);
int register_syscall(const char* name, syscall_fn_t func, int* nr);
int register_syscall_static(const char* name, syscall_fn_t func, int nr);

#define SYSCALL(n, arg) \
    asm("pushal"); \
    asm("mov %%ebx, %%eax" :: "b" (n)); \
    asm("mov %%ebx, %%ecx" :: "b" (arg) : "eax"); \
    asm("int $128"); \
    asm("popal");
