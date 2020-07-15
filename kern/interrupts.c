#include "interrupts.h"
#include "kernel.h"
#include "vga.h"
#include "pio.h"

idt_entry_t idt[INTR_COUNT];
intr_handler* bound_handlers[INTR_COUNT];

void make_gate(int i, void (*fn)(void), int dpl, int gate_type)
{
    idt[i].offset_low = (uint32_t)fn & 0xffff;
    idt[i].offset_high = ((uint32_t)fn & 0xffff0000) >> 16;
    idt[i].reserved0 = 0;
    idt[i].dpl = dpl;
    idt[i].gate_type = gate_type;
    idt[i].present = 1;
    idt[i].selector = KERNEL_CSEL;
}

void register_handler(int int_no, intr_handler* handler)
{
    if (int_no >= 0 && int_no < INTR_COUNT)
    {
        bound_handlers[int_no] = handler;
    }
}

void interrupts_init()
{
    interrupts_pic_init();

    for (int i = 0; i < INTR_COUNT; i++)
    {
        make_gate(i, interrupts_stubs[i], 0, 15);
        bound_handlers[i] = 0;
    }

    idt_descriptor_t descriptor = { sizeof(idt) - 1, (uint32_t)idt };
    asm volatile("lidt %0; sti" :: "m" (descriptor));
}

void interrupts_eoi(int irq)
{
    outb(PIC0_REG_CTRL, 0x20);

    if (irq >= 0x28)
        outb(PIC1_REG_CTRL, 0x20);
}

void interrupts_handle_int(uint32_t intr_num, uint32_t err_code) 
{
    if (bound_handlers[intr_num])
        bound_handlers[intr_num](intr_num, err_code);

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