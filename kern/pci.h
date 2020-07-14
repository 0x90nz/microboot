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

void pci_test();
uint16_t pci_cfg_read_word(uint8_t bus, uint8_t device, uint8_t func, uint8_t off);
uint32_t pci_cfg_read_dword(uint8_t bus, uint8_t device, uint8_t func, uint8_t off);