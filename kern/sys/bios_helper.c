#include "../stdlib.h"
#include "gdt.h"
#include "bios.h"

extern void _internal_bios_interrupt(uint8_t intr_num);
extern uint8_t bios_regs;

void int_dump_regs(struct int_regs* frame)
{
    debugf("eax: %08x ebx: %08x", frame->eax, frame->ebx);
    debugf("ecx: %08x edx: %08x", frame->ecx, frame->edx);
    debugf("esi: %08x edi: %08x", frame->esi, frame->edi);
    debugf("efl: %08x", frame->flags);
    debugf("esp: %08x", frame->esp);

    debugf("ds: %04x", frame->ds);
    debugf("es: %04x fs: %04x gs: %04x", frame->es, frame->fs, frame->gs);
}

void bios_interrupt(int number, struct int_regs* regs)
{
    int_dump_regs(regs);
    memcpy(&bios_regs, regs, sizeof(struct int_regs));
    _internal_bios_interrupt(number);
    memcpy(regs, &bios_regs, sizeof(struct int_regs));
}