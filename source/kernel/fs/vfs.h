#ifndef SOS_VFS_H
#define SOS_VFS_H

#include "../device/device.h"
#include "../lib/container/array_list/array_list.h"
#include "../lib/container/linked_list/linked_list.h"
#include "../lib/ref_count/ref_count.h"
#include "../lib/types.h"
#include "../synchronization/spin_lock.h"

struct vfs_dentry;
struct vfs_inode;
struct vfs;

typedef struct {
    // file operations
    u64 (*open)(struct vfs_inode* vn, int flags);
    u64 (*close)(struct vfs_inode* vn);
    u64 (*read)(struct vfs_inode* vn, u64 off, u8* buffer);
    u64 (*write)(struct vfs_inode* vn, u64 off, u8* buffer);

    // directory operations
    int (*lookup)(struct vfs_inode* dir, string name,
                  struct vfs_inode** result);

    int (*v_create)(struct vfs_inode* dir, string name, struct vattr* vattr,
                    struct vfs_inode** result);

    int (*v_mknod)(struct vfs_inode* dir, string name, struct vattr* vattr,
                   u64 dev, struct vfs_inode** result);

    int (*v_symlink)(struct vfs_inode* dir, string name, struct vattr* vattr,
                     string target, struct vfs_inode** result);

    int (*v_hardlink)(struct vfs_inode* dir, string name, struct vnode* target,
                      struct vfs_inode** result);

    int (*v_unlink)(struct vfs_inode* dir, struct vnode* vn,
                    struct vfs_inode* ve);

    int (*v_mkdir)(struct vfs_inode* dir, string name, struct vattr* vattr,
                   struct vfs_inode** result);

    int (*v_rmdir)(struct vfs_inode* dir, struct vnode* vn,
                   struct vfs_inode* ve);

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
    u64 id;

    vfs_inode_type type;
    vfs_inode_ops* ops;
    void* private_data;
} vfs_inode;

typedef struct {
    string name;

    vfs_inode* inode;
    struct vfs* fs;

    lock lock;
    bool dead;
    ref_count refc;

    union {
        // for devices
        device* dev;

        // for directories
        linked_list children; // contains `self_node`s of subdir dentries

        // for mountpoints
        struct vfs* mounted_here;
    };

    linked_list_node self_node;
} vfs_entry;

typedef struct {
} vfs_ops;

typedef struct {
    string name;

    vfs_ops* ops;
} vfs_type;

typedef struct {
    vfs_type* type;

    device* device;

    vfs_entry* mount_point;
    vfs_entry* root;
} vfs;

#endif // SOS_VFS_H
