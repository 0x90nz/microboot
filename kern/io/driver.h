#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "chardev.h"
#include "console.h"

enum device_type {
    DEVICE_TYPE_UNKNOWN = -1,
    // Console device
    DEVICE_TYPE_CON = 1,
    // Framebuffer device
    DEVICE_TYPE_FRAMEBUFFER,
    // Character device
    DEVICE_TYPE_CHAR,
    // Block device
    DEVICE_TYPE_BLOCK,
};

// Defines a device. And function pointer MAY be null to indicate that the
// function is not supported
struct device {
    char name[64];
    // Remove the device
    void (*destroy)(struct device*);
    // Set some parameter of the device, implementation specific
    int (*setparam)(struct device*, int param_id, void* aux);
    // Internal device, e.g. for a character device this would be a chardev_t
    void* internal_dev;
    // Private device information
    void* device_priv;
    // Type of device
    enum device_type type;
    // Any devices which depend on this device. This may be null to indicate
    // that no devices depend on this device.
    // On destruction of this device, all the subdevices will be destroyed first
    // and only once they are all destroyed will this device be destroyed.
    // Subdevices must be acyclic, avoid self-referential and cyclic
    // dependencies.
    // Called when devices are deregistered, or as a consequence above.
    struct device** subdevices;
    // The number of subdevices that depend on this device.
    size_t num_subdevices;
};

struct driver {
    char name[64];
    // Probe MUST be defined for any driver. Proble may be called multiple times
    // for instance, if another device that would enable some other class to
    // work has been found.
    void (*probe)(struct driver* driver);
    enum device_type type_for;
    bool first_probe;
    void* driver_priv;
};

void driver_init();
void driver_register(struct driver* driver);
bool driver_deregister(struct driver* driver);
bool device_deregister(struct device* device);
bool device_deregister_subdevices(struct device* device);
void device_register(struct device* device);
int device_get_first_available_suffix(const char* prefix);

void device_foreach(void (*fn)(struct device*));
void driver_foreach(void (*fn)(struct driver*));
struct device* device_firstmatch(bool (*pred)(const struct device*));

void driver_probe_for(enum device_type type);

struct device* device_get_by_name(const char* name);
chardev_t* device_get_chardev(struct device* dev);
console_t* device_get_console(struct device* dev);
// TODO: get_blockdev & blockdev itself
