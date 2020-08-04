#include "../stdlib.h"
#include "gdt.h"
#include "bios.h"

extern void _internal_bios_interrupt(uint8_t intr_num);
extern uint8_t bios_regs;

void bios_interrupt(int number, struct int_regs* regs)
{
    memcpy(&bios_regs, regs, sizeof(struct int_regs));
    _internal_bios_interrupt(number);
    memcpy(regs, &bios_regs, sizeof(struct int_regs));
}