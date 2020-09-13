#pragma once
#include <stdint.h>

int do_syscall(uint32_t nr, int arg0, int arg1, int arg2, int arg3, int arg4);

#define SYSCALL1(nr, arg) do_syscall(nr, (int)arg, 0, 0, 0, 0)
#define SYSCALL2(nr, arg0, arg1) do_syscall(nr, (int)arg0, (int)arg1, 0, 0, 0)
#define SYSCALL3(nr, arg0, arg1, arg2) do_syscall(nr, (int)arg0, (int)arg1, (int)arg2, 0, 0)
#define SYSCALL4(nr, arg0, arg1, arg2, arg3) do_syscall(nr, (int)arg0, (int)arg1, (int)arg2, (int)arg3, 0)
#define SYSCALL5(nr, arg0, arg1, arg2, arg3, arg4) do_syscall(nr, (int)arg0, (int)arg1, (int)arg2, (int)arg3, (int)arg4)

/*
 * The USE macro defines a function pointer kexp_[name] and a matching
 * trampoline function which will bounce all calls to the original function
 * through the function pointer. This relies on the function pointer having
 * been properly initialised.
 */
#define USE(n) typeof(n) *kexp_ ## n; \
asm( \
    ".text\n\t" \
    ".global " #n "\n\t" \
    #n ":\n\t" \
    "jmp *(kexp_" #n ")\n\t" \
)

/*
 * Initialises a function pointer as required by the USE macro 
 */
#define INIT(n) kexp_ ## n = (void*)SYSCALL1(getexport, #n);
