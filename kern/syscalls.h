#pragma once

#include <stdint.h>
#include "sys/syscall.h"

#define REGISTER_SYSCALL(name) \
    register_syscall(# name, syscall_ ## name, NULL);

void syscalls_init();