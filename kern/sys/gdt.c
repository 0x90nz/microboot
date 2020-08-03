#include "gdt.h"
#include "../stdlib.h"

#define GDT_TYPE(rw, dc, executable, segtype, priv) \
        ((rw & 1) << 1 \
        | (dc & 1) << 2 \
        | (executable & 1) << 3 \
        | (segtype & 1) << 4 \
        | (priv & 2) << 6 \
        | 1 << 7) \

static struct gdt_entry gdt[] = {
    {0, 0, 0},
    {0, 0xffffffff, GDT_TYPE(1, 0, 1, 1, 0), GDT_SIZE},      // 0x08, kernel code
    {0, 0xffffffff, GDT_TYPE(1, 0, 0, 1, 0), GDT_SIZE},      // 0x10, kernel data
};

// Create a buffer for our GDT. It is 8-byte aligned as per vol3 3.5.1 intel
// developer manual
uint8_t gdt_buffer[(sizeof(gdt) / sizeof(struct gdt_entry)) * 8] __attribute__((aligned(8)));

__attribute__((naked)) static void load_gdt(struct gdt_ptr* gdt)
{
    asm("mov    4(%esp), %eax");
    asm("lgdt   (%eax)");
    asm("ret");
}

/**
 * Encode a gdt_entry struct into a uint8_t array suitable for use with
 * 'lgdt'. The array should be 8 bytes long (for one entry)
 */
static void encode_entry(uint8_t* dst, struct gdt_entry src)
{
    if (src.limit > 65536) {
        src.limit = src.limit >> 12;
        dst[6] = (GDT_GRANULARITY);
    } else {
        dst[6] = 0;
    }

    // The 'access byte'
    dst[5] = src.type;

    // Encode the source address
    dst[0] = src.limit & 0xff;          // The first byte of limit
    dst[1] = (src.limit >> 8) & 0xff;   // The high byte of limit
    dst[6] |= (src.limit >> 16) & 0x0f; // The high nibble or limit
    
    dst[6] |= src.flags & 0xf0;

    // Encode the base address
    dst[2] = src.base & 0xff;           // Byte 1
    dst[3] = (src.base >> 8) & 0xff;    // Byte 2
    dst[4] = (src.base >> 16) & 0xff;   // Byte 3
    dst[7] = (src.base >> 24) && 0xff;  // Byte 4
}

static void encode()
{
    for (int i = 0; i < sizeof(gdt) / sizeof(struct gdt_entry); i++) {
        encode_entry(&gdt_buffer[i * 8], gdt[i]);
    }
}

void gdt_init()
{
    encode();
    struct gdt_ptr pointer;
    pointer.base = (uint32_t)gdt_buffer;
    pointer.limit = sizeof(gdt_buffer) - 1;
    debugf("Loaded GDT at %08x with %d entries", gdt_buffer, sizeof(gdt) / sizeof(struct gdt_entry));
    load_gdt(&pointer);
}