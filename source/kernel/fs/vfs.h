#ifndef SOS_VFS_H
#define SOS_VFS_H

#include "../device/device.h"
#include "../lib/container/array_list/array_list.h"
#include "../lib/container/hash_table/hash_table.h"
#include "../lib/container/linked_list/linked_list.h"
#include "../lib/ref_count/ref_count.h"
#include "../lib/types.h"
#include "../synchronization/spin_lock.h"

struct vfs_type;
struct vfs_super_block;
struct vfs_inode;
struct vfs_dentry;
struct vfs_path;

typedef struct {
    void (*evict)(struct vfs_inode* inode);

    // file operations
    u64 (*open)(struct vfs_inode* inode, int flags);
    u64 (*close)(struct vfs_inode* inode);
    u64 (*read)(struct vfs_inode* inode, u64 off, u8* buffer);
    u64 (*write)(struct vfs_inode* inode, u64 off, u8* buffer);

    // directory operations
    u64 (*unlink)(struct vfs_inode* dir, struct vfs_dentry* dentry);
    struct vfs_dentry* (*lookup)(struct vfs_dentry* parent, string name);
    u64 (*rename)(struct vfs_dentry* old_parent_dentry,
                  struct vfs_dentry* old_dentry,
                  struct vfs_dentry* victim_dentry,
                  struct vfs_dentry* new_dentry, string name);
} vfs_inode_ops;

typedef enum {
    FILE,
    DIRECTORY,
    CHARACTER_DEVICE,
    BLOCK_DEVICE,
    PIPE,
    SYMLINK,
    SOCKET
} vfs_inode_type;

typedef struct {
    struct vfs_dentry* (*mount)(struct vfs_type* type, device* dev);
    u64 (*unmount)(struct vfs_super_block* sb);
    u64 (*sync)(struct vfs_super_block* sb);
} vfs_type_ops;

typedef struct vfs_type {
    string name;

    vfs_type_ops* ops;

    lock lock; // guards all fields below
    linked_list super_blocks;
    ref_count refc;
} vfs_type;

void vfs_init();

bool register_vfs_type(vfs_type* type);
void deregister_vfs_type(vfs_type* type);

vfs_type* vfs_type_get(string name);
vfs_type* vfs_type_acquire(vfs_type* type);
void vfs_type_release(vfs_type* type);

u64 vfs_unlink(struct vfs_path start, string path);
u64 vfs_rename(struct vfs_path old_dir, struct vfs_dentry* source,
               struct vfs_path new_dir, string name);

#endif // SOS_VFS_H
