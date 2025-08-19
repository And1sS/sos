#ifndef SOS_RAMFS_H
#define SOS_RAMFS_H

#include "../../device/device.h"
#include "../vfs.h"

void ramfs_init();
struct vfs_inode* ramfs_mount(struct vfs_super_block* fs, device* dev);

#endif // SOS_RAMFS_H
