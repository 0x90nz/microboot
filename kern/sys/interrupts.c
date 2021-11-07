#include "interrupts.h"
#include "../kernel.h"
#include "../stdlib.h"
#include "../io/ansi_colours.h"
#include "../io/pio.h"
#include "../backtrace.h"

idt_entry_t idt[INTR_COUNT] __attribute__((aligned(8)));

/**
 * Interrupt Handlers:
 * Each interrupt may have _either_ a bound handler, or a low-level bound
 * handler associated with it. If a bound handler is present, it takes priority
 * over a low level handler.
 *
 * A low level handler has access to the entire state of the interrupted code
 * (all registers). By default all interrupts have a low level handler bound
 * to `exception`, which enforces that if an interrupt is created, it _must_
 * be handled, by registering either a high or low level handler.
 *
 * The reason for the separation is that a low level handler may modify the
 * state, and at the end of the interrupt, the state will be restored.
 * Therefore they are much more dangerous than a normal handler, and should
 * only be used when absolutely required.
 */

intr_handler* bound_handlers[INTR_COUNT];
ll_intr_handler* ll_bound_handlers[INTR_COUNT];

static char* exception_names[] = {
    "#DE",
    "#DB",
    "NMI",
    "#BP",
    "#OF",
    "#BR",
    "#UD",
    "#NM",
    "#DF",
    "#MF",
    "#TS",
    "#NP",
    "#SS",
    "#GP",
    "#PF",
    "RESERVED",
    "#MF",
    "#AC",
    "#MC",
    "#XM",
    "#VE",
    "#CP",
};

void dump_regs(intr_frame_t* frame)
{
    printf("eax: %08x ebx: %08x\n", frame->eax, frame->ebx);
    printf("ecx: %08x edx: %08x\n", frame->ecx, frame->edx);
    printf("esi: %08x edi: %08x\n", frame->esi, frame->edi);
    printf("eip: %08x efl: %08x\n", frame->eip, frame->eflags);
    printf("esp: %08x\n", frame->esp);

    printf("cs: %04x ss: %04x ds: %04x\n", frame->cs, frame->ss, frame->ds);
    printf("es: %04x fs: %04x gs: %04x\n", frame->es, frame->fs, frame->gs);

    dprintf_raw(ANSI_COLOR_RED "\nregister dump:\n");
    dprintf_raw("eax: %08x ebx: %08x\n", frame->eax, frame->ebx);
    dprintf_raw("ecx: %08x edx: %08x\n", frame->ecx, frame->edx);
    dprintf_raw("esi: %08x edi: %08x\n", frame->esi, frame->edi);
    dprintf_raw("eip: %08x efl: %08x\n", frame->eip, frame->eflags);
    dprintf_raw("esp: %08x\n", frame->esp);

    dprintf_raw("cs: %04x ss: %04x ds: %04x\n", frame->cs, frame->ss, frame->ds);
    dprintf_raw("es: %04x fs: %04x gs: %04x\n", frame->es, frame->fs, frame->gs);
    dprintf_raw(ANSI_COLOR_RESET);

    backtrace();
}

// Simple panic function that we bind by default
void exception(intr_frame_t* frame)
{
    printf("\n!!! Unhandled interrupt [%s] !!!\n", 
        frame->int_no < 22 ? exception_names[frame->int_no] : "no name");

    printf("int num: %d, err code: %08x\n", frame->int_no, frame->err_code);

    dump_regs(frame);
    hang();
}

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
    if (int_no >= 0 && int_no < INTR_COUNT) {
        bound_handlers[int_no] = handler;
    }
}

/**
 * Registers a low level handler. See comment at top for details.
 */
void register_ll_handler(int int_no, ll_intr_handler* handler)
{
    if (int_no >= 0 && int_no < INTR_COUNT) {
        ll_bound_handlers[int_no] = handler;
    }
}

void do_nothing(uint32_t int_no, uint32_t err_no) {}

void interrupts_init()
{
    asm("cli");
    disable_pic();

    for (int i = 0; i < INTR_COUNT; i++) {
        make_gate(i, interrupts_stubs[i], 0, 15);
        bound_handlers[i] = 0;
        register_ll_handler(i, exception);
    }

    // Register a handler for irq0, because the
    // timer is probably already running
    register_handler(IRQ_TO_INTR(0), do_nothing);
    register_ll_handler(3, dump_regs);

    interrupts_pic_init();

    idt_descriptor_t descriptor = { sizeof(idt) - 1, (uint32_t)idt };
    asm volatile("lidt %0; sti" :: "m" (descriptor));
}

void interrupts_eoi(int irq)
{
    outb(PIC0_REG_CTRL, 0x20);

    if (irq >= 0x28)
        outb(PIC1_REG_CTRL, 0x20);
}

void interrupts_handle_int(intr_frame_t* frame)
{
    if (bound_handlers[frame->int_no])
        bound_handlers[frame->int_no](frame->int_no, frame->err_code);
    else if (ll_bound_handlers[frame->int_no])
        ll_bound_handlers[frame->int_no](frame);

    if (frame->int_no >= 0x20 && frame->int_no < 0x30) {
        interrupts_eoi(frame->int_no);
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

unsigned char get_pic_mask(int pic)
{
	return inb(pic > 0 ? 0x21 : 0xa1);
}

void set_pic_mask(int pic, unsigned char mask)
{
	outb(pic > 0 ? 0x21 : 0xa1, mask);
}
