#include "interrupts.h"
#include "kernel.h"
#include "vga.h"

idt_entry_t idt[INTR_COUNT];

void make_gate(int i, void (*fn)(void), int dpl, int gate_type)
{
    idt[i].offset_low = (uint32_t)fn & 0xffff;
    idt[i].offset_high = ((uint32_t)fn & 0xffff0000) >> 16;
    idt[i].reserved0 = 0;
    idt[i].type_attr = 0x8e;
    idt[i].selector = 0x08;
}

void interrupts_init()
{
    interrupts_pic_init();

    for (int i = 0; i < INTR_COUNT; i++)
    {
        make_gate(i, interrupts_stubs[i], 0, 0);
    }

    idt_descriptor_t descriptor = { sizeof(idt) - 1, (uint32_t)idt };
    asm volatile("lidt %0" :: "m" (descriptor));
    asm("sti");
}

void interrupts_handle_int(uint32_t intr_num, uint32_t err_code) 
{
    if (intr_num >= 0x20 && intr_num < 0x30)
    {
        interrupts_eoi(intr_num);
    }
}

void disable_pic()
{
    outb(PIC0_REG_DATA, 0xff);
    outb(PIC1_REG_DATA, 0xff);
}

void interrupts_pic_init()
{
    outb(PIC0_REG_DATA, 0xff);
    outb(PIC1_REG_DATA, 0xff);

    outb(PIC0_REG_CTRL, 0x11);
    outb(PIC0_REG_DATA, 0x20);
    outb(PIC0_REG_DATA, 0x04);
    outb(PIC0_REG_DATA, 0x01);

    outb(PIC1_REG_CTRL, 0x11);
    outb(PIC1_REG_DATA, 0x28);
    outb(PIC1_REG_DATA, 0x02);
    outb(PIC1_REG_DATA, 0x01);

    outb(PIC0_REG_DATA, 0);
    outb(PIC1_REG_DATA, 0);
}

void interrupts_eoi(int irq)
{
    outb(PIC0_REG_CTRL, 0x20);

    if (irq >= 0x28)
        outb(PIC1_REG_CTRL, 0x20);
}