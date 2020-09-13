#include "elf.h"
#include "../stdlib.h"
#include "../alloc.h"

size_t get_elf_size(struct elf_header* hdr)
{
    struct elf_program_header* phdr = (struct elf_program_header*)((void*)hdr + hdr->phoff);
    size_t total_size = 0;
    for (int i = 0; i < hdr->phnum; i++, phdr++) {
        switch (phdr->type) {
        case ELF_PT_LOAD:
            total_size += phdr->memsz;
            break;
        default:
            break;
        }
    }

    return total_size;
}

void elf_load_mod(void* elf, symtab_handler add_to_symtab)
{
    struct elf_header* hdr = (struct elf_header*)elf;
    struct elf_program_header* phdr = elf + hdr->phoff;
    struct elf_section_header* shdr = elf + hdr->shoff;

    // Allocate memory to copy the segments into
    void* base = kalloc(get_elf_size(hdr));

    // Load all the loadable segments
    for (int i = 0; i < hdr->phnum; i++, phdr++) {
        switch (phdr->type) {
        case ELF_PT_LOAD:
            memcpy(base + phdr->paddr, elf + phdr->offset, phdr->filesz);
            break;
        default:
            break;
        }
    }

    uint32_t shstrtab_off = 0;
    struct elf_section_header* tmp = shdr;
    for (int i = 0; i < hdr->shnum; i++, tmp++) {
        // The SHT_STRTAB section
        if (tmp->type == 0x03)
            shstrtab_off = tmp->offset;
    }

    ASSERT(shstrtab_off, "Offset was zero for shstrtab");
    
    tmp = shdr;
    for (int i = 0; i < hdr->shnum; i++, tmp++) {        
        const char* name = elf + shstrtab_off + tmp->name;
        if (strcmp(name, "exports") == 0) {
            add_to_symtab(base, elf + tmp->offset, tmp->size);
            break;
        }
    }

    void (*entry)(void*) = base + hdr->entry;
    if (entry)
        entry(base);

    kfree(base);
}

void elf_run(void* elf, int argc, char** argv)
{
    struct elf_header* hdr = elf;
    struct elf_program_header* phdr = elf + hdr->phoff;

    // Allocate memory to copy the segments into
    void* base = kalloc(get_elf_size(hdr));

    // Load all the loadable segments
    for (int i = 0; i < hdr->phnum; i++, phdr++) {
        switch (phdr->type) {
        case ELF_PT_LOAD:
            memcpy(base + phdr->paddr, elf + phdr->offset, phdr->filesz);
            break;
        default:
            break;
        }
    }

    void (*entry)(int, char**) = (void (*)(int, char**))(base + hdr->entry);
    entry(argc, argv);

    kfree(base);
}