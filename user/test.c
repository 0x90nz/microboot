#include <stdarg.h>
#include "common.h"

void main()
{
    int puts = SYSCALL1(0, "puts");
    SYSCALL1(puts, "Hello!\n");
}