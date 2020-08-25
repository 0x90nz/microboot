#pragma once

typedef struct env_item {
    const char* key;
    void* value;
} env_item_t;

void env_put(const char* key, void* value);
void* _env_get(const char* key);


#define env_get(k, t)      ((t)_env_get(k))

void env_init();

#define ENV_SIZE        128