#pragma once
#include <stdint.h>

int do_syscall(uint32_t nr, int arg0, int arg1, int arg2, int arg3, int arg4);

#define SYSCALL1(nr, arg) do_syscall(nr, (int)arg, 0, 0, 0, 0)
#define SYSCALL2(nr, arg0, arg1) do_syscall(nr, (int)arg0, (int)arg1, 0, 0, 0)
#define SYSCALL3(nr, arg0, arg1, arg2) do_syscall(nr, (int)arg0, (int)arg1, (int)arg2, 0, 0)
#define SYSCALL4(nr, arg0, arg1, arg2, arg3) do_syscall(nr, (int)arg0, (int)arg1, (int)arg2, (int)arg3, 0)
#define SYSCALL5(nr, arg0, arg1, arg2, arg3, arg4) do_syscall(nr, (int)arg0, (int)arg1, (int)arg2, (int)arg3, (int)arg4)