#pragma once

#include <stdint.h>

#define PCI_CFG_ADDR        0xcf8
#define PCI_CFG_DATA        0xcfc

#define PCI_REG(reg, off)   (reg * 4 + off)

typedef struct {
    uint8_t reg_off     : 8;
    uint8_t func_num    : 3;
    uint8_t dev_num     : 5;
    uint8_t bus_num     : 8;
    uint8_t reserved0   : 7;
    uint8_t enable      : 1;
} __attribute__((packed)) pci_addr_t;

typedef struct {
    uint16_t vendor_id, device_id;
    uint16_t command, status;
    uint8_t revision_id, prog_if, subclass, class_code;
    uint8_t cache_line_size, latency_timer, header_type, bist;
    uint32_t bar0;
    uint32_t bar1;
    uint32_t bar2;
    uint32_t bar3;
    uint32_t bar4;
    uint32_t bar5;
    uint32_t cardbus_cis_pointer;
    uint16_t subsys_vendor_id, subsys_id;
    uint32_t expansion_rom_addr;
    uint8_t cap_pointer;
    uint8_t reserved1; uint16_t reserved2;
    uint32_t reserved3;
    uint8_t interrupt_line, interrupt_pin, min_grant, max_latency;
} __attribute__((packed)) pci_device_desc_t;

void pci_test();
uint16_t pci_cfg_read_word(uint8_t bus, uint8_t device, uint8_t func, uint8_t off);
uint32_t pci_cfg_read_dword(uint8_t bus, uint8_t device, uint8_t func, uint8_t off);
void pci_cfg_write_dword(uint8_t bus, uint8_t device, uint8_t func, uint8_t off, uint32_t data);