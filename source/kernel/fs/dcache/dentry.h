#ifndef SOS_DENTRY_H
#define SOS_DENTRY_H

#include "../inode.h"
#include "../vfs.h"
#include "dcache.h"

// set when dentry is no longer reachable from cache and is
// about to be freed, this is needed so that traversal of
// children can check whether it can use this dentry or not
#define DENTRY_DYING (1 << 0)

typedef struct vfs_dentry {
    // Immutable data
    vfs_inode* inode;
    // End of immutable data

    // Dcache data. Dentry stays in dcache during whole lifetime unless it is
    // detached.
    dcache_hash_entry hash_entry; // used in dcache, guarded by its bucket lock
    linked_list_node unused_node; // used in dcaches 'unused' list

    // used in parent 'children' list, needed for fail-safe
    // establishing/invalidating of parent-child dentry relationship and does
    // not have effect on dentry lifetime - this means that this node should be
    // removed from parent 'children' list before parent change occurs:
    // 1) when changing parent
    // 2) when destroying dentry
    linked_list_node dentry_node;

    u64 flags; // Should be accessed atomically

    lock lock; // guards all fields below

    dcache_bucket* hash_bucket; // dcache hash table bucket where dentry
                                // resides, NULL if not hashed

    // these two are closely tied to hash_bucket, can be changed only during
    // creation or namespace changes. Should be changed under both dcache and
    // dentry locks (to synchronize with dentry release), and parent inode->mut
    // locked for write (to prevent concurrent namespace changes). Each time any
    // of these fields changes - dentry should be unhashed or rehashed.
    string name;

    struct vfs_dentry* parent; // never NULL(real parent or self-reference)

    // holds references to 'dentry_node's of children dentries, this means that
    // while parent->lock is held, all children are not freed, but to use any
    // dentry from this list it has to be inspected by taking
    linked_list children;

    ref_count refc;
} vfs_dentry;

vfs_dentry* vfs_dentry_create(vfs_dentry* parent, vfs_inode* inode,
                              string name);
vfs_dentry* vfs_dentry_create_root(vfs_inode* inode);

vfs_dentry* vfs_dentry_lookup(vfs_dentry* parent, string name);

// parent inode mutex should be held for write during this operation
void vfs_dentry_unlink(vfs_dentry* dentry);

// atomically replaces name in `new_parent` with `child` of `old parent` and
// unlinks existing dentry that is replaced, both old parent and new parent
// inode mutexes should be held during this operation
void vfs_dentry_move(vfs_dentry* new_parent, vfs_dentry* child, string name);

vfs_dentry* vfs_dentry_acquire(vfs_dentry* dentry);
void vfs_dentry_release(vfs_dentry* dentry);
vfs_dentry* vfs_dentry_get_parent(vfs_dentry* dentry);

bool vfs_dentry_is_root(vfs_dentry* dentry);
bool vfs_dentry_is_dir(vfs_dentry* dentry);

// used for rename operation to avoid loops in tree structure, sb->rename_mut
// should be held during this operation
bool vfs_dentry_is_ancestor(vfs_dentry* dentry, vfs_dentry* ancestor);
// inode mutex should be held during this operation
bool vfs_dentry_is_orphaned(vfs_dentry* dentry);

#endif // SOS_DENTRY_H
