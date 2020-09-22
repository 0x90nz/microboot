#include "stddef.h"

struct symbol
{
    const char* name;
    void* fn;
} __attribute__((packed));

// Define a module. A separate section is used because the order of variables is
// not guaranteed when optimisation is enabled
#define MODULE(name) struct symbol symbol_mod_ ## name __attribute__((section("exports.start"))) = { #name, NULL }; \
int getexport; \
void module_init() \
{ \
    getexport = SYSCALL1(0, "getmodsym"); \
}

#define EXPORT_SYM(name) struct symbol symbol_ ## name __attribute__((section("exports"))) = { #name, name }