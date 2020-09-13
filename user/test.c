#include <stdarg.h>
#include <export.h>
#include "common.h"

MODULE(TEST);

void main(int argc, char** argv)
{
    int puts = SYSCALL1(0, "puts");
    SYSCALL1(puts, "Filename was: ");
    SYSCALL1(puts, argv[1]);
    SYSCALL1(puts, "\n");
}

void hello()
{
    int puts = SYSCALL1(0, "puts");
    SYSCALL1(puts, "Hello World!");
}
EXPORT_SYM(hello);

void hello2()
{
    int puts = SYSCALL1(0, "puts");
    SYSCALL1(puts, "Hello (the second) World!");
}
EXPORT_SYM(hello2);
