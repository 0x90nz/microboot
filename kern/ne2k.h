#pragma once

#include <stdint.h>
#include "pci.h"

#define NE2K_REG_CMD            0x00
#define NE2K_REG_TX_STAT        0x04
#define NE2K_REG_INT_STAT       0x07

// Remote start address 0, low byte
#define NE2K_RSAR0              0x08
// Remote start address 1, high byte
#define NE2K_RSAR1              0x09

// Remote byte count register 0, low byte
#define NE2K_RBCR0              0x0a
// Remote byte count register 1, high byte
#define NE2K_RBCR1              0x0b

#define NE2K_REG_RX_CFG         0x0c
#define NE2K_REG_TX_CFG         0x0d
#define NE2K_REG_DATA_CFG       0x0e
#define NE2K_REG_INT_MASK       0x0f
#define NE2K_REG_DATA           0x10
#define NE2K_REG_RESET          0x1f
#define NE_IO_EXTENT    0x20

void ne2k_init(pci_device_desc_t* device);