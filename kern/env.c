#include "env.h"
#include "alloc.h"
#include "stdlib.h"

static env_item_t* items;
static int item_count;

void env_init()
{
    items = kcalloc(sizeof(env_item_t) * 5);
    item_count = 0;
}

static int env_index(const char* key)
{
    for (int i = 0; i < item_count; i++) {
        if (strcmp(key, items[i].key) == 0) {
            return i;
        }
    }
    return -1;
}

void env_put(const char* key, void* value)
{
    ASSERT(items, "Trued to 'put' before items was initialised");
    int index = env_index(key);
    if (index != -1) {
        items[index].value = value;
    } else {
        items[item_count].key = key;
        items[item_count++].value = value;
    }
}

void* _env_get(const char* key)
{
    ASSERT(items, "Tried to 'get' before items was initialised");
    int index = env_index(key);
    if (index != -1) 
        return items[index].value;
    else
        return NULL;
}