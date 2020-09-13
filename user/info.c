#include <stdarg.h>
#include <export.h>
#include <printf.h>
#include <stdlib.h>
#include "common.h"

MODULE(test_mod);
USE(printf);
USE(gets);

void main(void* base)
{
    module_init();
    INIT(printf);
    INIT(gets);
}

void stuff()
{
    printf("Hello World!");
}
EXPORT_SYM(stuff);