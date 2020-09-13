#include <stdarg.h>
#include <export.h>
#include <printf.h>
#include "common.h"

MODULE(test_mod);
USE(printf);

void main(void* base)
{
    module_init();
    INIT(printf);
}

void stuff()
{
    printf("Hello World!");
}
EXPORT_SYM(stuff);