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

// Export a symbol that is placed in the symbol table as to be callable by other
// code, be it in another module or directly within the kernel
#define EXPORT_SYM(name) struct symbol symbol_ ## name __attribute__((section("exports"))) = { #name, name }

#define EXPORT_MOD_INIT_PREFIX "__init$"

// Export a symbol as an init symnol, that is one which will be called when a
// module is loaded (before that modules entry point!) or if a kernel symbol, at
// the initialisation of the module system.
#define EXPORT_INIT(name) struct symbol symbol_ ## name __attribute__((section("exports"))) = { EXPORT_MOD_INIT_PREFIX #name, name }

#define EXPORT_MOD_EARLY_INIT_PREFIX "__einit$"

#define EXPORT_EARLY_INIT(name) struct symbol symbol_ ## name __attribute__((section("exports"))) = { EXPORT_MOD_EARLY_INIT_PREFIX #name, name }
