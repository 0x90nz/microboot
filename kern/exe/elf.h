#pragma once

#include <stdint.h>

struct elf_header
{
    uint8_t ident[4];
    uint8_t class;
    uint8_t data;
    uint8_t version;
    uint8_t os_abi;
    uint8_t abi_version;
    uint8_t reserved0[7];
    uint16_t type;
    uint16_t machine;
    uint32_t version1;
    uint32_t entry;
    uint32_t phoff;
    uint32_t shoff;
    uint32_t flags;
    uint16_t ehsize;
    uint16_t phentsize;
    uint16_t phnum;
    uint16_t shentsize;
    uint16_t shnum;
    uint16_t shstrndx;
} __attribute__((packed));

struct elf_program_header
{
    uint32_t type;
    uint32_t offset;
    uint32_t vaddr;
    uint32_t paddr;
    uint32_t filesz;
    uint32_t memsz;
    uint32_t flags; // Required?
    uint32_t align;
} __attribute__((packed));

enum {
    ELF_PT_NULL = 0,
    ELF_PT_LOAD,
    ELF_PT_DYNAMIC,
    ELF_PT_INTERP,
    ELF_PT_NOTE,
    ELF_PT_SHLIB,
    ELF_PT_PHDR,
    ELF_PT_TLS
};

struct elf_section_header
{
    uint32_t name;
    uint32_t type;
    uint32_t flags;
    uint32_t addr;
    uint32_t offset;
    uint32_t size;
    uint32_t link;
    uint32_t info;
    uint32_t addralign;
    uint32_t entsize;

} __attribute__((packed));

void elf_run(void* elf, int argc, char** argv);