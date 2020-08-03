#pragma once

#include <stdint.h>

#define ATA_IOBASE              0x1f0
#define ATA_SEC_IOBASE          0x170
#define ATA_CONTROL_BASE        0x3f6
#define ATA_CONTROL_SEC_BASE    0x376

#define REG_CTRL_ALTSTATUS      0x00
#define REG_CTRL_SOFTRESET      0x04

#define REG_IO_DATA             0x00
#define REG_IO_ERROR            0x01
#define REG_IO_FEATURES         0x01
#define REG_IO_SECCOUNT         0x02
#define REG_IO_LBA0             0x03
#define REG_IO_LBA1             0x04
#define REG_IO_LBA2             0x05
#define REG_IO_DEVSEL           0x06
#define REG_IO_COMMAND          0x07
#define REG_IO_STATUS           0x07
// LBA48
#define REG_IO_SECCOUNT1        0x08
#define REG_IO_LBA3             0x09
#define REG_IO_LBA4             0x0a
#define REG_IO_LBA5             0x0b

#define STATUS_BSY              (1 << 7)
#define STATUS_DRDY             (1 << 6)
#define STATUS_DF               (1 << 5)
#define STATUS_DSC              (1 << 4)
#define STATUS_DRQ              (1 << 3)
#define STATUS_CORR             (1 << 2)
#define STATUS_IDX              (1 << 1)
#define STATUS_ERR              (1 << 0)

#define ATA_PRIMARY             0x00
#define ATA_SECONDARY           0x01

#define ATA_MASTER              0x00
#define ATA_SLAVE               0x01