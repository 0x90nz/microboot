#include <export.h>
#include "mod.h"
#include "list.h"
#include "exe/elf.h"
#include "printf.h"
#include "alloc.h"

struct list exports;
struct list modules;

void mod_init()
{
    list_init(&exports);
    list_init(&modules);
}

static void mod_symtab_add(void* base, void* symtab, size_t szsymtab)
{
    int num_syms = szsymtab / sizeof(struct symbol);
    struct symbol* sym = symtab;

    // First entry in exports is the module name
    sym->name += (uint32_t)base;
    sym->fn = base;
    list_append(&modules, list_node(sym));
    sym++;

    for (int i = 1; i < num_syms; i++, sym++) {
        // Apply relocations
        sym->name += (uint32_t)base;
        sym->fn += (uint32_t)base;
        list_append(&exports, list_node(sym));
    }
}

void mod_list_callback(struct list_node* node)
{
    struct symbol* mod = node->value;
    printf("%s: %08x\n", mod->name, mod->fn);
}

void mod_list()
{
    list_iterate(&modules, mod_list_callback);
}

void mod_sym_list()
{
    list_iterate(&exports, mod_list_callback);
}

void mod_load(void* module)
{
    elf_load_mod(module, mod_symtab_add);
}