#include "stddef.h"

struct symbol
{
    const char* name;
    void* fn;
} __attribute__((packed));

#define MODULE(name) struct symbol symbol_mod_ ## name __attribute__((section("exports"))) = { #name, NULL }
#define EXPORT_SYM(name) struct symbol symbol_ ## name __attribute__((section("exports"))) = { #name, name }