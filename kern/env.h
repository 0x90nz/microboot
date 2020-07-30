#pragma once

typedef struct env_item {
    const char* key;
    const char* value;
} env_item_t;

void env_put(const char* key, const char* value);
const char* env_get(const char* key);
void env_init();

#define ENV_SIZE        128