#pragma once

#include <stdint.h>
#include "pci.h"

#define NE2K_REG_CMD            0x00
#define NE2K_REG_TX_STAT        0x04
#define NE2K_REG_TPSR           0x04

#define NE2K_REG_TBCR0          0x05
#define NE2K_REG_TBCR1          0x06

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

#define NE2K_CMD_START          (1 << 1)
#define NE2K_CMD_TXP            (1 << 2)
#define NE2K_CMD_REMOTE_READ    (1 << 3)
#define NE2K_CMD_REMOTE_WRITE   (1 << 4)
#define NE2K_CMD_SEND_PKT       (3 << 4)

void ne2k_init(pci_device_desc_t* device);
void ne2k_select_page(int page);