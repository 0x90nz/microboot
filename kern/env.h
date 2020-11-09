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

void env_put(env_t* env, const char* key, void* value);
void* _env_get(env_t* env, const char* key);
void* env_remove(env_t* env, const char* key);


#define env_get(e, k, t)      ((t)_env_get(e, k))

env_t* env_init();