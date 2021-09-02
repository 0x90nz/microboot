#pragma once

typedef struct htbl htbl_t;

/**
 * @brief Creates a new hash table
 *
 * @return pointer to the new table if created, NULL on failure
 */
htbl_t* htbl_create();

/**
 * @brief Destroy a hash table. Will destroy the keys allocated, but will /NOT/
 * destroy the values within the table.
 *
 * @param tbl the table to destroy
 */
void htbl_destroy(htbl_t* table);

/**
 * @brief Get a value from the table, given a specific key.
 *
 * @param table the table to search within
 * @param key the key to search for
 * @return pointer to the mapped value for the given key if found, NULL
 * otherwise
 */
void* htbl_get(htbl_t* table, const char* key);

/**
 * @brief Create a mapping between the given key and value.
 *
 * @param table the table to create the mapping in
 * @param key the key to map the value to; copied so value need not persist
 * @param value the value to map the key to. /NOT/ copied, and must persist
 * @return non-zero value if the mapping was created. zero on failure
 */
int htbl_put(htbl_t* table, const char* key, void* value);

/**
 * Typed table get macro. For convenience mostly, equivalent to `htbl_get` and
 * a cast to the desired type.
 */
#define htbl_tget(tbl, k, type) (type)htbl_get(tbl, k)

