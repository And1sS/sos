#ifndef SOS_VFS_H
#define SOS_VFS_H

#include "../device/device.h"
#include "../lib/container/array_list/array_list.h"
#include "../lib/container/hash_table/hash_table.h"
#include "../lib/container/linked_list/linked_list.h"
#include "../lib/ref_count/ref_count.h"
#include "../lib/types.h"
#include "../synchronization/spin_lock.h"

#define VFS_PARENT_NAME ".."

struct vfs_inode;
struct vfs_dentry;
struct vfs;

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

typedef struct {
    struct vfs_inode* (*mount)(struct vfs_super_block* vfs, device* device);
    u64 (*unmount)(struct vfs_super_block* vfs);
    u64 (*sync)(struct vfs_super_block* vfs);
    //    u64 (*stat)(struct vfs_super_block* vfs_super_block, struct vfs_stat*
    //    stat);
} vfs_ops;

typedef struct {
    string name;

    vfs_ops* ops;
} vfs_type;

typedef struct {
    u64 id;
    vfs_type* type;

    device* device;

    struct vfs_dentry* root;
} vfs_super_block;

struct vfs_dentry* walk(struct vfs_dentry* start, string path);

#endif // SOS_VFS_H
