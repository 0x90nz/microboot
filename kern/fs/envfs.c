/**
 * @file envfs.c
 * @brief A filesystem which wraps an environment. The environment must only
 * contain pointers to strings as the values. Pointers to binary data are
 * *currently* not supported, as internally this uses strlen which will fail
 * on a null char. Eventually the idea is to support any arbitrary data. 
 */

#include "envfs.h"
#include "../alloc.h"
#include "../env.h"
#include "../stdlib.h"

/*
 * WIP: environment filesystem. Needs far more error checking, which will
 * probably require a bit of a rethink of env, as there's currently no
 * magic number like thing I can check. Either that or just store some
 * metadata to ensure silly things don't happen 
 */

struct envfs_priv {
    env_t* env;
};

static fs_file_priv_t getfile(fs_t* fs, fs_file_priv_t file, const char* name)
{
    env_t* env = file;
    return _env_get(env, name);
}

static uint32_t read(fs_t* fs, fs_file_priv_t file, uint32_t offset, size_t len, void* buffer)
{
    size_t max_len = strlen(file);
    if (offset > max_len)
        return 0;

    if (max_len - offset > len)
        len -= (max_len - len);

    memcpy(buffer, file + offset, len);
    return len;
}

static fs_file_priv_t get_root(fs_t* fs)
{
    struct envfs_priv* priv = fs->fs_priv;
    return priv->env;
}

static void _ls_callback(const char* key, void* value)
{
    printf("%s\n", key);
}

static void ls(fs_t* fs, fs_file_priv_t file)
{
    env_t* env = file;
    env_iterate(env, _ls_callback);
}

static uint32_t fsize(fs_t* fs, fs_file_priv_t file)
{
    const char* str = file;
    return strlen(str);
}

/**
 * @brief Initialise a new environment filesystem
 * 
 * @param env the environment to mirror
 * @return fs_t* the filesystem mirroring that environment
 */
fs_t* envfs_init(env_t* env)
{
    fs_ops_t* ops = kallocz(sizeof(*ops));
    ops->getfile = getfile;
    ops->read = read;
    ops->get_root = get_root;
    ops->ls = ls;
    ops->fsize = fsize;

    struct envfs_priv* priv = kalloc(sizeof(*priv));
    priv->env = env;

    fs_t* fs = kalloc(sizeof(*fs));
    fs->ops = ops;
    fs->fs_priv = priv;
    fs->type = FS_ENVFS;

    return fs;
}
