#pragma once

#include <stdint.h>

typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t reserved0;
    // uint8_t type_attr;
    uint8_t gate_type       : 4;
    uint8_t storage_segment : 1;
    uint8_t dpl             : 2;
    uint8_t present         : 1;
    uint16_t offset_high;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_descriptor_t;


/*
* eflags
* cs
* eip
* err code
* int num
* eax
* ecx
* edx
* ebx
* esp
* ebp
* esi
* edi
 */

typedef struct {
    uint16_t gs, fs, es, ds, ss;
    uint32_t edi, esi, ebp, ebx, edx, ecx, eax, esp;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags;    
} __attribute__((packed)) intr_frame_t;

typedef void intr_handler (uint32_t int_no, uint32_t err_no);
typedef void ll_intr_handler (intr_frame_t* frame); // Low level handler, gets the entire frame

typedef void intr_stub (void);
extern intr_stub* interrupts_stubs[256];

void disable_pic();
void interrupts_init();
void interrupts_pic_init();
uint64_t make_idt_descriptor(uint16_t limit, void* base);
void register_handler(int int_no, intr_handler* handler);

#define IRQ_TO_INTR(x)  (x + 32)

#define INTR_COUNT      256
#define PIC0_REG_CTRL   0x20
#define PIC0_REG_DATA   0x21
#define PIC1_REG_CTRL   0xa0
#define PIC1_REG_DATA   0xa1