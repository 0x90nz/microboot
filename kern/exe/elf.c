#include "elf.h"
#include "../stdlib.h"

void elf_run(void* elf)
{
    struct elf_header* hdr = (struct elf_header*)elf;
    struct elf_program_header* phdr = (struct elf_program_header*)(elf + hdr->phoff);
    for (int i = 0; i < hdr->phnum; i++, phdr++) {
        switch (phdr->type) {
        case ELF_PT_LOAD:
            memcpy((void*)phdr->paddr, elf + phdr->offset, phdr->filesz);
            break;
        default:
            break;
        }
    }

    void (*entry)(void) = (void (*)(void))hdr->entry;
    entry();
}