#ifndef SOS_RAMFS_H
#define SOS_RAMFS_H

#include "../../device/device.h"
#include "../vfs.h"

#define RAMFS_NAME "ramfs"

void ramfs_init();
struct vfs_dentry* ramfs_mount(struct vfs_type* type, device* dev);

#endif // SOS_RAMFS_H
