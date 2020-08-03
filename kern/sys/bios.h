#pragma once

#include <stdint.h>

struct int_regs
{
	uint32_t edi, esi, ebp, esp;
	uint32_t ebx, edx, ecx, eax;
	uint16_t flags;
	uint16_t es, ds, fs, gs;
} __attribute__ ((packed));

void bios_interrupt(int number, struct int_regs* regs);