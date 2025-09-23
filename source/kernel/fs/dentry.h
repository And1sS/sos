#ifndef SOS_DENTRY_H
#define SOS_DENTRY_H

#include "inode.h"
#include "vfs.h"

typedef struct vfs_dentry {
    // Immutable data
    string name;
    vfs_inode* inode;
    // End of immutable data

    lock lock;     // guards all fields below
    bool dying;    // set when dentry is no longer reachable from cache and is
                   // about to be freed, this is needed so that traversal of
                   // children can check whether it can use this dentry or not
    bool detached; // set when dentry is detached (from rmdir or unlink) and
                   // should not stay in cache after refcount reaches 0 even if
                   // cache is not full

    struct vfs_dentry* parent; // never NULL(real parent or self-reference)
    linked_list_node dentry_node; // used in parents 'children' list
    linked_list_node unused_node; // used in dcache 'unused' list
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

void vfs_dcache_init(u64 max_entries);
// parent reference should be held during this routine
vfs_dentry* vfs_dcache_get(vfs_dentry* parent, string name);

#endif // SOS_DENTRY_H
