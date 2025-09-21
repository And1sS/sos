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

typedef struct {
    // file operations
    u64 (*open)(struct vfs_inode* vn, int flags);
    u64 (*close)(struct vfs_inode* vn);
    u64 (*read)(struct vfs_inode* vn, u64 off, u8* buffer);
    u64 (*write)(struct vfs_inode* vn, u64 off, u8* buffer);

    // directory operations
    u64 (*unlink)(struct vfs_inode* dir, struct vfs_dentry* dentry);
    struct vfs_dentry* (*lookup)(struct vfs_dentry* parent, string name);
    //
    //    u64 (*create)(struct vfs_inode* dir, string name, struct vattr* vattr,
    //                  struct vfs_entry** result);
    //
    //    u64 (*mknod)(struct vfs_inode* dir, string name, struct vattr* vattr,
    //                 u64 dev, struct vfs_entry** result);
    //
    //    u64 (*symlink)(struct vfs_inode* dir, string name, struct vattr*
    //    vattr,
    //                   string target, struct vfs_entry** result);
    //
    //    u64 (*hardlink)(struct vfs_inode* dir, string name, struct vnode*
    //    target,
    //                    struct vfs_entry** result);
    //
    //    u64 (*unlink)(struct vfs_inode* dir, struct vnode* vn,
    //                  struct vfs_entry* ve);
    //
    //    u64 (*mkdir)(struct vfs_inode* dir, string name, struct vattr* vattr,
    //                 struct vfs_entry** result);
    //
    //    u64 (*rmdir)(struct vfs_inode* dir, struct vnode* vn, struct
    //    vfs_entry* ve);

    // int (*v_rename)(struct vnode *dir, struct vnode *vn, struct ventry
    // *old_ve, struct vnode *new_dir, cstr_t new_name);
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
    u64 (*fill_super)(struct vfs_super_block* sb, device* dev);
    struct vfs_dentry* (*mount)(struct vfs_type* type, device* dev);
    u64 (*unmount)(struct vfs_super_block* sb);
    u64 (*sync)(struct vfs_super_block* sb);
} vfs_type_ops;

struct vfs_type {
    string name;

    vfs_type_ops* ops;

    lock lock; // guards all fields below
    linked_list super_blocks;
    ref_count refc;
};

void vfs_init();

bool register_vfs_type(struct vfs_type* type);
void deregister_vfs_type(struct vfs_type* type);

#endif // SOS_VFS_H
