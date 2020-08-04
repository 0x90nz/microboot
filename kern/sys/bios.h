#pragma once

#include <stdint.h>

struct int_regs
{
	uint32_t edi, esi, ebp, esp;
	uint32_t ebx, edx, ecx, eax;
	uint16_t flags;
	uint16_t es, ds, fs, gs;
} __attribute__ ((packed));

enum flags_register {
    EFL_CF = 0x0001,
    EFL_PF = 0x0004,
    EFL_AF = 0x0010,
    EFL_ZF = 0x0040,
    EFL_SF = 0x0080,
    EFL_TF = 0x0100,
    EFL_IF = 0x0200,
    EFL_DF = 0x0400,
    ELF_OF = 0x0800,
    // IOPL stuff might go here at some point

    EFL_RF = 0x00010000,
    EFL_VM = 0x00020000,
    EFL_AC = 0x00040000,
    EFL_VIF = 0x00080000,
    EFL_VIP = 0x00100000,
    EFL_ID = 0x00200000
};

void bios_interrupt(int number, struct int_regs* regs);
extern uint8_t low_mem_buffer;