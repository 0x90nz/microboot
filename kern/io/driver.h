#pragma once

#include <stdbool.h>
#include <stddef.h>

#include "chardev.h"
#include "blkdev.h"
#include "console.h"

enum device_type {
    DEVICE_TYPE_UNKNOWN = -1,
    DEVICE_TYPE_NONE = 0,
    // Console device
    DEVICE_TYPE_CON = 1,
    // Framebuffer device
    DEVICE_TYPE_FRAMEBUFFER,
    // Character device
    DEVICE_TYPE_CHAR,
    // Block device
    DEVICE_TYPE_BLOCK,
    // Filesystem device
    DEVICE_TYPE_FS,
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
    // A probe MUST be defined for any driver. Probe may be called multiple
    // times for instance, if another device that would enable some other class
    // to work has been found.
    //
    // If probe_directed is defined, then probe may be left undefined, and
    // vice-versa.
    void (*probe)(struct driver* driver);
    // A version of probe in which the invoking driver is passed in (useful for
    // devices which would be sub-devices). If no invoking device exists (for
    // instance on the initial probe), invoker will be NULL.
    void (*probe_directed)(struct driver* driver, struct device* invoker);
    // The type of device this driver is for
    enum device_type type_for;
    // The type (if any) of device that this driver depends on (for instance a
    // framebuffer console might depend on a framebuffer). If this is defined,
    // probe_directed MUST be used instead of probe.
    enum device_type depends_on;
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

void driver_probe_for(enum device_type type, struct device* invoker);

struct device* device_get_by_name(const char* name);
chardev_t* device_get_chardev(struct device* dev);
console_t* device_get_console(struct device* dev);
blkdev_t* device_get_blkdev(struct device* dev);

