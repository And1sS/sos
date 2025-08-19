#ifndef SOS_DCACHE_H
#define SOS_DCACHE_H

#include "icache.h"
#include "vfs.h"

 struct vfs_dentry {
    struct dentry* parent;
    string name;
    vfs_inode* inode;

    lock lock; // guards all fields below
    ref_count refc;
};

// parent reference should be held during this routine
struct vfs_dentry* vfs_dentry_create(struct vfs_dentry* parent, vfs_inode* inode,
                              string name);
struct vfs_dentry* vfs_dentry_make_root(vfs_inode* inode);

void vfs_dentry_destroy(struct vfs_dentry* dentry);

void vfs_dentry_acquire(struct vfs_dentry* dentry);
void vfs_dentry_release(struct vfs_dentry* dentry);

void vfs_dentry_attach(struct vfs_dentry* dentry, vfs_inode* inode);

void vfs_dcache_init();
// parent reference should be held during this routine
struct vfs_dentry* vfs_dcache_get(struct vfs_dentry* parent, string name);

bool vfs_dcache_put(struct vfs_dentry* dentry);

#endif // SOS_DCACHE_H
