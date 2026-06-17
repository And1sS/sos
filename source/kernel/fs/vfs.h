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

void vfs_init();

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
