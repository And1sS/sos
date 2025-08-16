#ifndef SOS_DCACHE_H
#define SOS_DCACHE_H

#include "../lib/container/hash_table/hash_table.h"
#include "inode.h"
#include "vfs.h"

/*
 * dcache is a hash table:
 * directory inode* to hash table (name to inode*)
 */
vfs_inode* dcache_get(vfs_inode* dir, string name);

#endif // SOS_DCACHE_H
