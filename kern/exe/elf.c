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

void elf_run(void* elf)
{
    struct elf_header* hdr = (struct elf_header*)elf;
    struct elf_program_header* phdr = (struct elf_program_header*)(elf + hdr->phoff);
    void* base = kalloc(get_elf_size(hdr));
    for (int i = 0; i < hdr->phnum; i++, phdr++) {
        switch (phdr->type) {
        case ELF_PT_LOAD:
            memcpy(base + phdr->paddr, elf + phdr->offset, phdr->filesz);
            break;
        default:
            break;
        }
    }

    void (*entry)(void) = (void (*)(void))(base + hdr->entry);
    entry();

    kfree(base);
}