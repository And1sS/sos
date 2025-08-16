//#include "vfs.h"
//#include "../error/errno.h"
//#include "../error/error.h"
//#include "../lib/string.h"
//#include "vfs_entry_registry.h"
//
//static vfs root_fs;
//static lock vfs_lock = SPIN_LOCK_STATIC_INITIALIZER;
//
//u64 walk(vfs_entry* start, string path, vfs_entry** result);
//
//void vfs_mount(struct vfs* vfs, device* device, string path) {
//    vfs_entry* entry;
//    u64 result = walk(root_fs.root, path);
//}
//
//u64 split_length(string path) {
//    u64 len = 0;
//    for (; path[len] != '/' && path[len] != '\0'; len++)
//        ;
//
//    return len;
//}
//
//void next_split(string path, u8 buffer[256], u8* result_length) {
//    // TODO: skip consequent slashes
//
//    *result_length = split_length(path);
//    memcpy(buffer, (void*) path, *result_length);
//    buffer[*result_length] = '\0';
//}
//
//u64 walk(vfs_entry* start, string path, vfs_entry** result) {
//    u64 full_length = strlen(path);
//    u64 walked_length = 0;
//
//    u8 split_buffer[256];
//    string path_iter = path;
//    vfs_entry* iter = start;
//
//    while (iter) {
//        u8 length;
//        next_split(path_iter, split_buffer, &length);
//        walked_length += length;
//        path_iter += length;
//
//        if (walked_length == full_length) {
//            *result = iter;
//            return 0;
//        }
//
//        if (iter->inode->type != DIRECTORY)
//            return -ENOTDIR;
//
//        u64 lookup_result = iter->inode->ops->lookup(
//            (struct vfs_inode*) iter->inode, path, (struct vfs_entry**) &iter);
//
//        if (IS_ERROR(lookup_result))
//            return lookup_result;
//
//        if (!iter)
//            return -ENOTDIR;
//    }
//
//    __builtin_unreachable();
//}