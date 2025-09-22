#ifndef SOS_DENTRY_H
#define SOS_DENTRY_H

#include "inode.h"
#include "vfs.h"

typedef struct vfs_dentry {
    // Immutable data
    string name;
    vfs_inode* inode;
    // End of immutable data

    lock lock; // guards all fields below
    bool dead;

    struct vfs_dentry* parent;
    linked_list_node dentry_node; // used in parents 'children' list
    linked_list children;

    ref_count refc;
} vfs_dentry;

// parent reference should be held during this routine
vfs_dentry* vfs_dentry_create(vfs_dentry* parent, vfs_inode* inode,
                              string name);
vfs_dentry* vfs_dentry_create_root(vfs_inode* inode);

// inode mutex should be held for write during this operation
void vfs_dentry_delete(vfs_dentry* dentry);

void vfs_dentry_acquire(vfs_dentry* dentry);
void vfs_dentry_release(vfs_dentry* dentry);
vfs_dentry* vfs_dentry_get_parent(vfs_dentry* dentry);

void vfs_dcache_init();
// parent reference should be held during this routine
vfs_dentry* vfs_dcache_get(vfs_dentry* parent, string name);

#endif // SOS_DENTRY_H
