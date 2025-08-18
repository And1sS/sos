#ifndef SOS_ICACHE_H
#define SOS_ICACHE_H

#include "inode.h"

bool icache_init(u64 max_inodes);

vfs_inode* icache_get(vfs* fs, u64 id);
vfs_inode* icache_release(vfs_inode* inode);

#endif // SOS_ICACHE_H
