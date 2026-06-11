#ifndef SOS_VFS_H
#define SOS_VFS_H

#include "../device/device.h"
#include "../lib/container/array_list/array_list.h"
#include "../lib/container/hash_table/hash_table.h"
#include "../lib/container/linked_list/linked_list.h"
#include "../lib/ref_count/ref_count.h"
#include "../lib/types.h"
#include "../memory/virtual/umem.h"
#include "../synchronization/spin_lock.h"

struct vfs_type;
struct vfs_super_block;
struct vfs_inode;
struct vfs_dentry;
struct vfs_path;
struct vfs_file;

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

// returns resolved mount root
struct vfs_path vfs_root();

struct vfs_file* vfs_open(struct vfs_path start, string path, u64 flags);
struct vfs_file* vfs_create(struct vfs_path path, string name, u64 mode);

// Consumes and releases file reference after closing
u64 vfs_close(struct vfs_file* file);

struct vfs_file* vfs_mkdir(struct vfs_path path, string name, u64 flags,
                           u64 mode);
u64 vfs_rmdir(struct vfs_path path);
u64 vfs_listdir(struct vfs_path path);

u64 vfs_unlink(struct vfs_path start, string path);
u64 vfs_rename(struct vfs_path old_dir, struct vfs_dentry* source,
               struct vfs_path new_dir, string name);

u64 vfs_read(struct vfs_file* file, __user void* buf, u64 size);
u64 vfs_write(struct vfs_file* file, __user void* buf, u64 size);

#endif // SOS_VFS_H
