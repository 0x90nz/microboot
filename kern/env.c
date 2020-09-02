/**
 * @file env.c
 * @brief An environment which stores a pointer to something keyed by a string
 */

#include "env.h"
#include "alloc.h"
#include "stdlib.h"


/**
 * @brief Initialise the environment
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
        if (strcmp(key, env->items[i].key) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Put an item into the environment
 * 
 * @param key the key which the item should be associated with
 * @param value the item
 */
void env_put(env_t* env, const char* key, void* value)
{
    ASSERT(env, "Trued to 'put' before environment");
    int index = env_index(env, key);
    if (index != -1) {
        env->items[index].value = value;
    } else {
        ASSERT(strlen(key) < ENV_KEY_LEN, "Key too long for environment");
        strcpy(env->items[env->current].key, key);
        env->items[env->current].value = value;
        env->current++;
    }
}

/**
 * @brief Get an item from the environment. Use the `env_get` macro to include
 * an automatic cast to the desired type
 * 
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