#include "../kernel.h"
#include "../stdlib.h"
#include "pio.h"
#include "pci.h"
#include "../net/ne2k.h"

void read_dev(uint32_t* desc, uint8_t bus, uint8_t device, uint8_t func)
{
    for (int i = 0; i < 16; i++)
    {
        desc[i] = pci_cfg_read_dword(bus, device, func, PCI_REG(i, 0));
    }
}

void pci_check_func(uint8_t bus, uint8_t device, uint8_t func)
{
    uint16_t vid = pci_cfg_read_word(bus, device, func, 2);
    uint16_t did = pci_cfg_read_word(bus, device, func, 0);

    uint32_t buf[16];
    if (vid == 0x10ec && did == 0x8029)
    {
        read_dev(buf, bus, device, func);
        pci_device_desc_t* dev = (pci_device_desc_t*)buf;
        ne2k_init(dev);
    }
}

uint8_t pci_get_header_type(uint8_t bus, uint8_t device, uint8_t func)
{
    return pci_cfg_read_word(bus, device, func, PCI_REG(3, 2));
}

void pci_check(uint8_t bus, uint8_t device)
{
    uint16_t vid = pci_cfg_read_word(bus, device, 0, 0);
    if (vid != 0xFFFF) {
        // The device actually exists, do something with int
        pci_check_func(bus, device, 0);

        // If this is a multi-function device, we need to check the rest of the
        // functions that it might support
        if (pci_get_header_type(bus, device, 0) & 0x80) {
            for (int i = 1; i < 8; i++) {
                if (pci_cfg_read_word(bus, device, i, 0) != 0xFFFF) {
                    pci_check_func(bus, device, i);
                }
            }
        }
    }
}

void pci_enumerate()
{
    // Yeah, we could do fancy recursive stuff to figure out what we should
    // be scanning, but that's complicated, and I can't be bothered.
    // Brute force is fast enough for me.

    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t device = 0; device < 32; device++) {
            pci_check(bus, device);
        }
    }
}

void pci_test()
{
    pci_enumerate();
}


uint32_t pci_cfg_read_dword(uint8_t bus, uint8_t device, uint8_t func, uint8_t off)
{
    ASSERT((off & 3) == 0, "Unaligned PCI read attempt");

    pci_addr_t addr;
    addr.reserved0 = 0;

    addr.bus_num = bus;
    addr.dev_num = device;
    addr.func_num = func;
    addr.reg_off = off;
    addr.enable = 1;

    outl(PCI_CFG_ADDR, *(uint32_t*)&addr);
    return inl(PCI_CFG_DATA);
}

void pci_cfg_write_dword(uint8_t bus, uint8_t device, uint8_t func, uint8_t off, uint32_t data)
{
    ASSERT((off & 3) == 0, "Unaligned PCI read attempt");

    pci_addr_t addr;
    addr.reserved0 = 0;

    addr.bus_num = bus;
    addr.dev_num = device;
    addr.func_num = func;
    addr.reg_off = off;
    addr.enable = 1;
    outl(PCI_CFG_ADDR, *(uint32_t*)&addr);
    outl(PCI_CFG_DATA, data);
}

uint16_t pci_cfg_read_word(uint8_t bus, uint8_t device, uint8_t func, uint8_t off)
{
    uint32_t ret = pci_cfg_read_dword(bus, device, func, off & 0xfc);
    // Choose whether to return the high or low word from the given offset
    return off & 2 ? ret & 0xFFFF : ret >> 16;
}