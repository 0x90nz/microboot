#include "driver.h"
#include "../stdlib.h"
#include "../alloc.h"
#include "../list.h"

struct list drivers;
struct list devices;

/**
 * @brief Initialise the driver manager.
 *
 */
void driver_init()
{
    list_init(&drivers);
    list_init(&devices);
}

/**
 * @brief Register a device.
 *
 * @param device the device to register
 */
void device_register(struct device* device)
{
    debugf("registering device %s", device->name);
    list_append(&devices, list_node(device));

    LIST_FOREACH(current, &drivers) {
        struct driver* drv = list_value(current);
        if (drv->depends_on != DEVICE_TYPE_NONE && drv->depends_on == device->type) {
            ASSERT(drv->probe_directed, "Device defining depends_on must define probe_directed");
            drv->probe_directed(drv, device);
        }
    }
}

/**
 * @brief Deregister a single device, and all of its subdevices (if there are any).
 *
 * @param device the device to deregister
 * @return true if the device was deregistered
 * @return false if the device was not deregistered
 */
bool device_deregister(struct device* device)
{
    debugf("deregistering device %s", device->name);
    LIST_FOREACH(current, &devices) {
        if (device == list_value(current)) {
            // must remove subdevices first so that they don't continue with an
            // invalid device
            device_deregister_subdevices(device);

            // only now is it safe to destroy this device
            if (device->destroy)
                device->destroy(device);
            list_remove(current);
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Deregister any subdevices this device might have.
 *
 * The caller is responsible for deallocating any resources used by the
 * subdevices array, but the num_subdevices member *is* reset to 0.
 *
 * @param device the device to deregister subdevices for
 * @return true if any number of subdevices were deregistered
 * @return false if no subdevices were deregistered
 */
bool device_deregister_subdevices(struct device* device)
{
    if (!device->subdevices)
        return false;

    for (int i = 0; i < device->num_subdevices; i++) {
        struct device* subdevice = device->subdevices[i];
        device_deregister(subdevice);
    }
    device->num_subdevices = 0;
    return true;
}

/**
 * @brief Register a driver.
 *
 * @param driver the driver to register
 */
void driver_register(struct driver* driver)
{
    debugf("registering driver %s", driver->name);

    list_append(&drivers, list_node(driver));
    ASSERT(driver->probe || driver->probe_directed, "Driver must define probe");
    driver->first_probe = true;
    if (driver->probe) {
        driver->probe(driver);
    } else {
        if (driver->depends_on != DEVICE_TYPE_NONE) {
            LIST_FOREACH(current, &devices) {
                struct device* current_dev = list_value(current);
                if (current_dev->type == driver->depends_on) {
                    driver->probe_directed(driver, current_dev);
                }
            }
        } else {
            driver->probe_directed(driver, NULL);
        }
    }
    driver->first_probe = false;
}

/**
 * @brief Deregister a driver.
 *
 * @param driver the driver to deregister
 * @return true if the driver was deregistered
 * @return false if the driver wasn't deregistered
 */
bool driver_deregister(struct driver* driver)
{
    LIST_FOREACH(current, &drivers) {
        if (driver == list_value(current)) {
            list_remove(current);
            return true;
        }
    }
    return false;
}

/**
 * @brief Iterate over all of the devices.
 *
 * @param fn function pointer to be invoked for device encountered
 */
void device_foreach(void (*fn)(struct device*))
{
    LIST_FOREACH(current, &devices) {
        struct device* dev = list_value(current);
        fn(dev);
    }
}

/**
 * @brief Get the first matching device from a given function pointer predicate.
 *
 * @param pred predicate returning true if the device matches and false otherwise
 * @return struct device* the first device encountered for which pred returns true
 */
struct device* device_firstmatch(bool (*pred)(const struct device*))
{
    LIST_FOREACH(current, &devices) {
        struct device* dev = list_value(current);
        if (pred(dev))
            return dev;
    }
    return NULL;
}

/**
 * @brief Iterate over all the drivers.
 *
 * @param fn function pointer to be invoked for each driver encountered
 */
void driver_foreach(void (*fn)(struct driver*))
{
    LIST_FOREACH(current, &drivers) {
        struct driver* dev = list_value(current);
        fn(dev);
    }
}

/**
 * @brief Get a device by its name.
 *
 * @param name the name of the device
 * @return struct device* the first device matching name if present, NULL if no
 * such device exists.
 */
struct device* device_get_by_name(const char* name)
{
    LIST_FOREACH(current, &devices) {
        struct device* dev = list_value(current);
        if (strcmp(name, dev->name) == 0) {
            return dev;
        }
    }
    return NULL;
}

/**
 * @brief For a device name, get the first available suffix for a given prefix.
 *
 * @note for this to work all devices matching the prefix must follow the
 * conventional structure of having said prefix and then a decimal number
 *
 * @warning no more than 512 devices are supported.
 *
 * @param prefix the prefix for the device, e.g. "sp"
 * @return int the first unused suffix, i.e. if no devices use this prefix 0, if
 * one device exists named "<prefix>0" then 1, etc.
 */
int device_get_first_available_suffix(const char* prefix)
{
    uint8_t bitmap[64];
    memset(bitmap, 0, sizeof(bitmap));

    size_t prefix_len = strlen(prefix);
    LIST_FOREACH(current, &devices) {
        struct device* dev = list_value(current);
        if (strncmp(dev->name, prefix, prefix_len) == 0) {
            char* suffix = dev->name + prefix_len;
            int suffix_num = atoi(suffix);
            ASSERT(suffix_num < 512, "Suffix numbers greater than 512 not supported");
            bitmap[suffix_num / 8] |= 1 << (suffix_num % 8);
        }
    }

    for (int i = 0; i < 512; i++) {
        if (!(bitmap[i / 8] & (1 << i % 8))) {
            return i;
        }
    }

    // Should never reach here, but as an insurance policy
    return 512;
}

/**
 * @brief Re-probe for a specific type of device.
 *
 * @note this function is costly to call, probes for *any* matching driver will
 * be called.
 *
 * @param type the type of device to probe for.
 */
void driver_probe_for(enum device_type type, struct device* invoker)
{
    debugf("starting re-probe for type %d", type);
    LIST_FOREACH(current, &drivers) {
        struct driver* driver = list_value(current);
        if (driver->type_for == type) {
            if (driver->probe) {
                driver->probe(driver);
            } else {
                driver->probe_directed(driver, invoker);
            }
        }
    }
}

/**
 * @brief Get a chardev for the given device.
 *
 * @param dev the device to get a chardev from
 * @return chardev_t* the chardev if the device is a character device. If not, an
 * assertion will fail causing a kernel panic.
 */
chardev_t* device_get_chardev(struct device* dev)
{
    ASSERT(dev->type == DEVICE_TYPE_CHAR, "Device must be a chardev");
    return dev->internal_dev;
}

/**
 * @brief Get a console for the given device.
 *
 * @param dev the device to get a console from
 * @return console_t* the console device if the device is a console device. If
 * not, an assertion will fail causing a kernel panic.
 */
console_t* device_get_console(struct device* dev)
{
    ASSERT(dev->type == DEVICE_TYPE_CON, "Device must be a console");
    return dev->internal_dev;
}

blkdev_t* device_get_blkdev(struct device* dev)
{
    ASSERT(dev->type == DEVICE_TYPE_BLOCK, "Device must be a block device");
    return dev->internal_dev;
}

