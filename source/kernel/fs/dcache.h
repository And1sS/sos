#ifndef SOS_DCACHE_H
#define SOS_DCACHE_H

#include "../lib/container/hash_table/hash_table.h"
#include "dentry.h"
#include "inode.h"
#include "vfs.h"

typedef struct dentry {
    struct dentry* parent;
    string name;
    vfs_inode* inode;
} vfs_dentry;

/*
 * dcache is a hash table:
 * directory inode* to hash table (name to inode*)
 */
void dcache_init();

vfs_dentry* dentry_create();
void dentry_release(vfs_dentry* dentry);
void dentry_destroy(vfs_dentry* dentry);
void dentry_attach(vfs_dentry* dentry, vfs_inode* inode);

vfs_dentry* dcache_get(vfs_dentry* parent, string name);
bool dcache_put(vfs_dentry* dentry);

#endif // SOS_DCACHE_H
