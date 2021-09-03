#include "config.h"
#include "htbl.h"
#include "stdlib.h"
#include "alloc.h"

enum config_type {
    CONFIG_TYPE_INT = 1,
    CONFIG_TYPE_STR,
};

struct config_value {
    uint8_t type;
    union {
        int ival;
        const char* sval;
    };
};

static htbl_t* namespaces;

void config_init()
{
    namespaces = htbl_create();
}

void config_newns(const char* name)
{
    htbl_put(namespaces, name, htbl_create());
}

// parse a config key in the format <namespace>:<key>. returns zero on failure,
// non-zero on success, in which case `namespace` will refer to the namespace
// and `key` will refer to the key, allocated with `kalloc`
static int config_parse_key(const char* path, const char** namespace, const char** key)
{
    const char* sep = strchr(path, ':');
    if (!sep)
        return 0;

    size_t ns_length = sep - path;
    char* ns = kalloc(ns_length + 1);
    memcpy(ns, path, ns_length);
    ns[ns_length] = '\0';
    *namespace = ns;

    size_t key_len = strlen(sep + 1);
    char* k = kalloc(key_len + 1);
    memcpy(k, sep + 1, key_len);
    k[key_len] = '\0';
    *key = k;

    return 1;
}

static void* config_get_common(const char* ns, const char* ns_key)
{
    htbl_t* ns_table = htbl_get(namespaces, ns);
    return htbl_get(ns_table, ns_key);
}

void config_set_common(const char* ns, const char* ns_key, void* value)
{
    htbl_t* ns_table = htbl_get(namespaces, ns);
    htbl_put(ns_table, ns_key, value);
}

void config_setstrns(const char* ns, const char* key, const char* value)
{
    struct config_value* old_value = config_get_common(ns, key);
    if (old_value && old_value->type == CONFIG_TYPE_STR)
        kfree((void*)old_value->sval);

    struct config_value* conf_value = kalloc(sizeof(*conf_value));
    conf_value->type = CONFIG_TYPE_STR;
    conf_value->sval = strdup(value);
    config_set_common(ns, key, conf_value);
}

void config_setintns(const char* ns, const char* key, int value)
{
    struct config_value* conf_value = kalloc(sizeof(*conf_value));
    conf_value->type = CONFIG_TYPE_INT;
    conf_value->ival = value;
    config_set_common(ns, key, conf_value);
}

const char* config_getstrns(const char* ns, const char* key)
{
    struct config_value* conf_value = config_get_common(ns, key);
    if (!conf_value || conf_value->type != CONFIG_TYPE_STR)
        return NULL;
    return conf_value->sval;
}

int config_getintns(const char* ns, const char* key)
{
    struct config_value* conf_value = config_get_common(ns, key);
    if (!conf_value || conf_value->type != CONFIG_TYPE_INT)
        return 0;
    return conf_value->ival;
}

void config_setstr(const char* key, const char* value)
{
    const char* ns;
    const char* ns_key;
    if (!config_parse_key(key, &ns, &ns_key))
        return;
    if (!ns || !ns_key)
        return;

    config_setstrns(ns, ns_key, value);

    kfree((void*)ns);
    kfree((void*)ns_key);
}

void config_setint(const char* key, int value)
{
    const char* ns;
    const char* ns_key;
    if (!config_parse_key(key, &ns, &ns_key))
        return;
    if (!ns || !ns_key)
        return;

    config_setintns(ns, ns_key, value);

    kfree((void*)ns);
    kfree((void*)ns_key);
}

const char* config_getstr(const char* key)
{
    const char* ns;
    const char* ns_key;
    if (!config_parse_key(key, &ns, &ns_key))
        return NULL;
    if (!ns || !ns_key)
        return NULL;

    const char* ret = config_getstrns(ns, ns_key);

    kfree((void*)ns);
    kfree((void*)ns_key);

    return ret;
}

int config_getint(const char* key)
{
    const char* ns;
    const char* ns_key;
    if (!config_parse_key(key, &ns, &ns_key))
        return 0;
    if (!ns || !ns_key)
        return 0;

    int ret = config_getintns(ns, ns_key);

    kfree((void*)ns);
    kfree((void*)ns_key);

    return ret;
}

int config_existsns(const char* ns, const char* key)
{
    return config_get_common(ns, key) != NULL;
}

