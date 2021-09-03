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
 * @brief Create a new config entry for the given string
 *
 * @param key the key to create the entry for
 * @param value string to map the entry to
 */
void config_setstr(const char* key, const char* value);

void config_setstrns(const char* ns, const char* key, const char* value);
void config_setintns(const char* ns, const char* key, int value);
const char* config_getstrns(const char* ns, const char* key);
int config_getintns(const char* ns, const char* key);
int config_existsns(const char* ns, const char* key);

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

