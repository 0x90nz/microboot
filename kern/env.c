/**
 * @file env.c
 * @brief An environment which stores a pointer to something keyed by a string.
 * **not** hashed, so uses a linear search, so don't use for performance-sensitive
 * applications.
 */

#include "env.h"
#include "alloc.h"
#include "stdlib.h"

/**
 * @brief Initialise a new environment, will dynamically allocate memory
 * 
 * @return env_t* pointer to the new environment
 */
env_t* env_init()
{
    env_t* env = kallocz(sizeof(env_t));
    env->current = 0;
    env->max = ENV_SIZE;
    env->items = kallocz(sizeof(env_item_t) * ENV_SIZE);
    return env;
}

static int env_index(env_t* env, const char* key)
{
    for (int i = 0; i < env->current; i++) {
        if (env->items[i].status & ENV_PRESENT 
                && strcmp(key, env->items[i].key) == 0) {
            return i;
        }
    }
    return -1;
}

static int env_first_free(env_t* env)
{
    for (int i = 0; i < env->current; i++) {
        if (env->items[i].status & ENV_EMPTY)
            return i;
    }

    if (env->current >= env->max)
        return -1;

    return env->current++;
}

/**
 * @brief Put an item into the environment
 * 
 * @param env the environment
 * @param key the key which the item should be associated with
 * @param value the item
 */
void env_put(env_t* env, const char* key, void* value)
{
    ASSERT(env, "Tried to 'put' before environment was initialised");
    int index = env_index(env, key);
    if (index != -1) {
        env->items[index].value = value;
    } else {
        ASSERT(strlen(key) < ENV_KEY_LEN, "Key too long for environment");
        index = env_first_free(env);
        if (index != -1) {
            strcpy(env->items[index].key, key);
            env->items[index].value = value;
            env->items[index].status = ENV_PRESENT;
        } else {
            log(LOG_WARN, "env tried to put to full environment");
        }
    }
}

/**
 * @brief Remove an item from the environment
 * 
 * @param env the environment
 * @param key the key to remove
 * @return void* the value of the item removed
 */
void* env_remove(env_t* env, const char* key)
{
    ASSERT(env, "Tried to 'remove' before environment was initialised");
    int index = env_index(env, key);

    if (index != -1) {
        env_item_t* item = &env->items[index];
        item->status = ENV_EMPTY;
        return item;
    }

    return NULL;
}

/**
 * @brief Get an item from the environment. Use the `env_get` macro to include
 * an automatic cast to the desired type
 * 
 * @param env the environment
 * @param key the key which the item is associated with
 * @return void* a pointer to the items value. `NULL` if no item with such a
 * name exists
 */
void* _env_get(env_t* env, const char* key)
{
    ASSERT(env, "Tried to 'get' before environment was initialised");
    int index = env_index(env, key);
    if (index != -1) 
        return env->items[index].value;
    else
        return NULL;
}

/**
 * @brief Iterate over the present keys in the environment, calling
 * iter for each present key-value pair
 * 
 * @param iter the function to call on iteration
 */
void env_iterate(env_t* env, env_iter_func iter)
{
    for (int i = 0; i < env->current; i++) {
        if (env->items[i].status & ENV_PRESENT) {
            iter(env->items[i].key, env->items[i].value);
        }
    }
}

/**
 * @brief Add an equals ('=') separated key-value pair string to the
 * environment.
 *
 * NOTE: the value will not be copied, so any dynamically allocated memory
 * passed to this function should be considered used until the containing
 * environment is destroyed.
 *
 * @param env the environment to insert the kvp into
 * @param kvp_string the string containing a key and a value
 * @return int non-zero if the item was added; zero on failure.
 */
int env_kvp_str_add(env_t* env, char* kvp_string)
{
    char* mid = strchr(kvp_string, '=');
    if (!mid)
        return 0;

    *mid = '\0';

    env_put(env, kvp_string, (void*)mid + 1);
    return 1;
}

/**
 * @brief Add multiple lines of key-value separated text to an environment.
 *
 * The same note applies as to env_kvp_str_add.
 *
 * On failure, some items may still have been added.
 *
 * @param env the environment to add the kvp lines into
 * @param kvp_lines the lines too add
 * @return int non-zero if all items were added; zero on failure.
 */
int env_kvp_lines_add(env_t* env, char* kvp_lines)
{
    char* token = strtok(kvp_lines, "\n");
    while (token) {
        if (!env_kvp_str_add(env, token))
            return 0;
        token = strtok(NULL, "\n");
    }
    return 1;
}
