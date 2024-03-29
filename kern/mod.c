/**
 * @file mod.c
 * @brief Module system
 */

#include <export.h>
#include "mod.h"
#include "list.h"
#include "exe/elf.h"
#include "printf.h"
#include "alloc.h"

static struct list exports;
static struct list modules;


/**
 * @brief Initialise the module system
 * 
 */
void mod_init()
{
    list_init(&exports);
    list_init(&modules);
}

static void module_sym_add(struct symbol* sym)
{
    list_append(&exports, list_node(sym));
}

/**
 * @brief Do early init for exported symbols with the early init prefix.
 *
 * @param symtab the symbol table
 * @param symtab_size the size of the symbol table (in bytes)
 */
void mod_ksymtab_early_init(void* symtab, size_t symtab_size)
{
    int num_syms = symtab_size / sizeof(struct symbol);
    struct symbol* sym = symtab;

    for (int i = 0; i < num_syms; i++, sym++) {
        if (strncmp(EXPORT_MOD_EARLY_INIT_PREFIX, sym->name, strlen(EXPORT_MOD_EARLY_INIT_PREFIX)) == 0) {
            void (*fn)() = sym->fn;
            fn();
        }
    }
}

/**
 * @brief Add the kernel symbol table to the list of exports and initialise late
 * init exports.
 *
 * @param symtab the symbol table
 * @param symtab_size the size of the symbol table (in bytes)
 */
void mod_ksymtab_add(void* symtab, size_t symtab_size)
{
    int num_syms = symtab_size / sizeof(struct symbol);
    struct symbol* sym = symtab;

    for (int i = 0; i < num_syms; i++, sym++) {
        if (strncmp(EXPORT_MOD_INIT_PREFIX, sym->name, strlen(EXPORT_MOD_INIT_PREFIX)) == 0) {
            void (*fn)() = sym->fn;
            fn();
        } else {
            // only add if this is /not/ an early init symbol. normal init
            // symbols would have been caught above
            if (strncmp(EXPORT_MOD_EARLY_INIT_PREFIX, sym->name, strlen(EXPORT_MOD_EARLY_INIT_PREFIX)) != 0)
                module_sym_add(sym);
        }
    }
}

/**
 * @brief Add a symbol table from a module to the list of exports
 *
 * @param base the base address of the module
 * @param symtab the symbol table
 * @param szsymtab the size of the symbol table (in bytes)
 */
void mod_symtab_add(void* base, void* symtab, size_t szsymtab)
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

static void mod_list_callback(struct list_node* node)
{
    struct symbol* mod = node->value;
    printf("%-20s %08x\n", mod->name, mod->fn);
}

/**
 * @brief List all of the currently loaded modules
 * 
 */
void mod_list()
{
    list_iterate(&modules, mod_list_callback);
}

/**
 * @brief List all of the currently exported symbols
 * 
 */
void mod_sym_list()
{
    list_iterate(&exports, mod_list_callback);
}

/**
 * @brief Get a symbol from the currently exported symbols
 * 
 * @param name the name of the symbol
 * @return void* the pointer to the symbol
 */
void* mod_sym_get(const char* name)
{
    struct list_node* current = &exports.head;
    while ((current = list_next(current)) && current->next) {
        struct symbol* sym = current->value;
        if (strcmp(sym->name, name) == 0)
            return sym->fn;
    }

    return NULL;
}

/**
 * @brief Load an ELF module
 * 
 * @param module the module file contents, must be an ELF file
 */
void mod_load(void* module)
{
    elf_load_mod(module, mod_symtab_add);
}
