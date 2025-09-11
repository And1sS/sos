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
    SOCKET,
    MOUNTPOINT
} vfs_inode_type;

#define INODE_NEW_FLAG 1 << 0

typedef struct {
    bool (*fill_super)(struct vfs_super_block* vfs);
    struct vfs_dentry* (*mount)(struct vfs_super_block* vfs, device* device);
    u64 (*unmount)(struct vfs_super_block* vfs);
    u64 (*sync)(struct vfs_super_block* vfs);
} vfs_type_ops;

typedef struct {
    string name;

    vfs_type_ops* ops;

    lock lock; // guards all fields below
    linked_list super_blocks;
    ref_count refc;
} vfs_type;

void vfs_init();

bool register_vfs_type(vfs_type* type);
void deregister_vfs_type(vfs_type* type);

struct vfs_dentry* walk(struct vfs_dentry* start, string path);

#endif // SOS_VFS_H
