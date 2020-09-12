#include <stdarg.h>
#include "common.h"

void main(int argc, char** argv)
{
    int puts = SYSCALL1(0, "puts");
    SYSCALL1(puts, "Filename was: ");
    SYSCALL1(puts, argv[1]);
    SYSCALL1(puts, "\n");
}