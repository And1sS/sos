#ifndef SOS_FILE_H
#define SOS_FILE_H

#include "path.h"

#define O_CREAT (1 << 0)
#define O_TRUNC (1 << 1)
#define O_WRONLY (1 << 2)
#define O_RONLY (1 << 3)
#define O_DIRECTORY (1 << 4)
#define O_APPEND (1 << 5)

// file_ops are defined in inode.h

typedef struct vfs_file {
    // Immutable data
    vfs_path path;
    vfs_file_ops* ops;
    u64 flags;
    // End of immutable data

    u64 pos; // not guarded, it is up to user to synchronize accesses to file,
             // data corruption is not a kernel concern, kernel guards accesses
             // only to an actual inode, so fs implementations should carefully
             // inspect this field before using it

    void* private_data; // same

    u64 refc; // Should be accessed atomically
} vfs_file;

vfs_file* vfs_file_create(vfs_path path, u64 flags);

vfs_file* vfs_file_acquire(vfs_file* file);
void vfs_file_release(vfs_file* file);

void vfs_file_close(vfs_file* file);

#endif // SOS_FILE_H
