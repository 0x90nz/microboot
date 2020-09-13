#include "kfs.h"
#include "fs.h"
#include "../alloc.h"
#include "../list.h"

struct kfs_dir
{
    struct list files;
};

struct kfs_priv
{
    struct kfs_dir root_dir;
};

fs_t* kfs_init()
{
    fs_t* fs = kalloc(sizeof(*fs));
    fs->type = FS_KFS;

    fs_ops_t* ops = kalloc(sizeof(*ops));
    ops->destroy = NULL;
    ops->fsize = NULL;
    ops->get_root = NULL;
    ops->getfile = NULL;
    ops->ls = NULL;
    ops->read = NULL;
    fs->ops = ops;

    struct kfs_priv* priv = kalloc(sizeof(*priv));
    list_init(&priv->root_dir.files);
    fs->fs_priv = priv;

    return fs;
}
