#include <stdint.h>
#include "pio.h"

// Byte sized port access
void outb(uint16_t portnumber, uint8_t data)
{
	asm volatile("outb %%al, %%dx" :: "d" (portnumber), "a" (data));
}
 
uint8_t inb(uint16_t portnumber)
{
	uint8_t result;
	asm volatile("inb %%dx, %%al" : "=a" (result) : "d" (portnumber));
	return result;
}

// Word sized port access
uint16_t inw(uint16_t portnumber)
{
	uint16_t result;
	asm volatile("inw %%dx, %%ax" : "=a" (result) : "d" (portnumber));
	return result;
}

void outw(uint16_t portnumber, uint16_t data)
{
	asm volatile("outw %%ax, %%dx" :: "d" (portnumber), "a" (data));
}

// Long sized port access

uint32_t inl(uint16_t portnumber)
{
	uint32_t result;
	asm volatile("inl %%dx, %%eax" : "=a" (result) : "d" (portnumber));
	return result;
}

void outl(uint16_t portnumber, uint32_t data)
{
	asm volatile("outl %%eax, %%dx" :: "d" (portnumber), "a" (data));
}