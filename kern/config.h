#pragma once

/**
 * Configuration paths:
 * Config keys are in the format "<namespace>:<key>", where <namespace> is any
 * valid namespace registered with `config_newns`, and key is any valid c string
 *
 * Configuration is append or overwrite only. You cannot delete a key which
 * already exists.
 */

/**
 * @brief Initialise the config system
 *
 */
void config_init();

/**
 * @brief Create a new configuration namespace
 *
 * @param name the name for the new namespace
 */
void config_newns(const char* name);

/**
 * For all of the following "ns" variants, they are identical in all behaviour
 * to the below documented non-"ns" variants, except for the fact that they key
 * parameter is instead split into the individual components of "namespace"
 * (ns) and the actual qualified key.
 */

void config_setstrns(const char* ns, const char* key, const char* value);
void config_setintns(const char* ns, const char* key, int value);
void config_setobjns(const char* ns, const char* key, void* value);
const char* config_getstrns(const char* ns, const char* key);
int config_getintns(const char* ns, const char* key);
void* config_getobjns(const char* ns, const char* key);
int config_existsns(const char* ns, const char* key);

/**
 * @brief Create a new config entry for the given string
 *
 * @param key the key to create the entry for
 * @param value string to map the entry to
 */
void config_setstr(const char* key, const char* value);

/**
 * @brief Create a new config entry for the given void pointer
 *
 * Important note! Because of the nature of a void*, usage of this is prone to
 * error, especially if it is in any way unclear what they type of the value
 * would be. Thus, if you are simply mapping something like a string or integer
 * value it is STRONGLY preferable to use the specific functions provided for
 * this.
 *
 * @param key the key to create the entry for
 * @param value the pointer to map the entry to
 */
void config_setobj(const char* key, void* value);

/**
 * @brief Create a new config entry for the given integer value
 *
 * @param key the key to create the entry for
 * @param value the integer value to map the entry to
 */
void config_setint(const char* key, int value);

/**
 * @brief Get the string value of a configuration key
 *
 * @param key the key to get the string value from
 * @return the string value mapping of the key if it exists, NULL otherwise
 */
const char* config_getstr(const char* key);

/**
 * @brief Get the void pointer value for the given configuration key
 *
 * @param key the key to get the pointer from
 * @return the pointer mapping of the key if it exists, and is a pointer, NULL
 * otherwise
 */
void* config_getobj(const char* key);

/**
 * @brief Get the integer value of a configuration key
 *
 * @param key the key to get the integer value from
 * @return the integer value if the key exists, zero if not
 */
int config_getint(const char* key);

/**
 * @brief Check the presence of a given key
 *
 * @param key the key to check the presence of
 * @return non-zero if the key is present, zero otherwise
 */
int config_exists(const char* key);

