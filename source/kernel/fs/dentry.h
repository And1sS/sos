#ifndef SOS_DENTRY_H
#define SOS_DENTRY_H

#include "inode.h"
#include "vfs.h"

struct vfs_dentry {
    // Immutable data
    string name;
    vfs_inode* inode;
    // End of immutable data

    lock lock; // guards all fields below
    bool dying;

    struct vfs_dentry* parent;
    linked_list_node dentry_node; // used in parents 'children' list
    linked_list children;

    ref_count refc;
};

// parent reference should be held during this routine
struct vfs_dentry* vfs_dentry_create(struct vfs_dentry* parent,
                                     vfs_inode* inode, string name);
struct vfs_dentry* vfs_dentry_create_root(vfs_inode* inode);

void vfs_dentry_acquire(struct vfs_dentry* dentry);
void vfs_dentry_release(struct vfs_dentry* dentry);
struct vfs_dentry* vfs_dentry_get_parent(struct vfs_dentry* dentry);

void vfs_dcache_init();
// parent reference should be held during this routine
struct vfs_dentry* vfs_dcache_get(struct vfs_dentry* parent, string name);

#endif // SOS_DENTRY_H
