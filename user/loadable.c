#include <stdint.h>
#include "loadable.h"
#include "../kern/sys/syscall.h"

START_SECT uint32_t size = (uint32_t)&LOADABLE_SIZE;

int init()
{
    SYSCALL(0, "Hello!\n");
    return 1234;
}