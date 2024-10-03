#ifndef SOS_VFS_H
#define SOS_VFS_H

#include "../device/device.h"
#include "../lib/container/array_list/array_list.h"
#include "../lib/container/linked_list/linked_list.h"
#include "../lib/ref_count/ref_count.h"
#include "../lib/types.h"
#include "../synchronization/spin_lock.h"

struct vfs_entry;
struct vfs_inode;
struct vfs;

typedef u64 lookup_func(struct vfs_inode* dir, string name,
                   struct vfs_entry** result);

typedef struct {
    // file operations
    u64 (*open)(struct vfs_inode* vn, int flags);
    u64 (*close)(struct vfs_inode* vn);
    u64 (*read)(struct vfs_inode* vn, u64 off, u8* buffer);
    u64 (*write)(struct vfs_inode* vn, u64 off, u8* buffer);

    // directory operations
    lookup_func* lookup;

    u64 (*create)(struct vfs_inode* dir, string name, struct vattr* vattr,
                  struct vfs_entry** result);

    u64 (*mknod)(struct vfs_inode* dir, string name, struct vattr* vattr,
                 u64 dev, struct vfs_entry** result);

    u64 (*symlink)(struct vfs_inode* dir, string name, struct vattr* vattr,
                   string target, struct vfs_entry** result);

    u64 (*hardlink)(struct vfs_inode* dir, string name, struct vnode* target,
                    struct vfs_entry** result);

    u64 (*unlink)(struct vfs_inode* dir, struct vnode* vn,
                  struct vfs_entry* ve);

    u64 (*mkdir)(struct vfs_inode* dir, string name, struct vattr* vattr,
                 struct vfs_entry** result);

    u64 (*rmdir)(struct vfs_inode* dir, struct vnode* vn, struct vfs_entry* ve);

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

typedef enum { UNLOADED, LOADED, DEAD } vfs_entry_state;

typedef struct {
    string name;

    vfs_inode* inode;
    struct vfs* fs;

    lock lock;
    vfs_entry_state state;
    ref_count refc;

    union {
        // for devices
        device* dev;

        // for mountpoints
        struct vfs* mounted_here;
    };

    linked_list_node self_node;
} vfs_entry;

typedef struct {
    u64 (*mount)(struct vfs* vfs, device* device, struct vfs_entry** root);
    u64 (*unmount)(struct vfs* vfs);
    u64 (*sync)(struct vfs* vfs);
    u64 (*stat)(struct vfs* vfs, struct vfs_stat* stat);
} vfs_ops;

typedef struct {
    string name;

    vfs_ops* ops;
} vfs_type;

typedef struct {
    u64 id;
    vfs_type* type;

    device* device;

    vfs_entry* mount_point;
    vfs_entry* root;
} vfs;

#endif // SOS_VFS_H
