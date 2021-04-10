#pragma once

#define ENV_SIZE        128
#define ENV_KEY_LEN     64

#define ENV_PRESENT     (1 << 0)
#define ENV_EMPTY       (1 << 1)

typedef struct env_item {
    char status;
    char key[ENV_KEY_LEN];
    void* value;
} env_item_t;

typedef struct env {
    int max;
    int current;
    env_item_t* items;
} env_t;

typedef void (*env_iter_func)(const char* k, void* v);

void env_put(env_t* env, const char* key, void* value);
void* _env_get(env_t* env, const char* key);
void* env_remove(env_t* env, const char* key);
void env_iterate(env_t* env, env_iter_func iter);
int env_kvp_str_add(env_t* env, char* kvp_string);
int env_kvp_lines_add(env_t* env, char* kvp_lines);

#define env_get(e, k, t)      ((t)_env_get(e, k))

env_t* env_init();