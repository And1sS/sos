#include "ramfs.h"
#include "../../error/errno.h"
#include "../../error/error.h"
#include "../../lib/alignment.h"
#include "../../lib/math.h"
#include "../dcache/dentry.h"
#include "../file.h"
#include "internal_tree.h"

static vfs_dentry* ramfs_mount(struct vfs_type* type, device* dev);

static void ramfs_evict(vfs_inode* inode);
static vfs_dentry* ramfs_create(vfs_dentry* parent, string name, u64 flags);

static u64 ramfs_open(vfs_file* file, u64 mode);
static u64 ramfs_write(vfs_file* file, __user void* buf, u64 size);
static u64 ramfs_read(vfs_file* file, __user void* buf, u64 size);

vfs_dentry* ramfs_lookup(vfs_dentry* parent, string name);
static u64 ramfs_rename(vfs_dentry* old_dir, vfs_dentry* source,
                        vfs_dentry* new_dir, vfs_dentry* target, string name);
static u64 ramfs_unlink(vfs_inode* dir, vfs_dentry* child);

static vfs_type_ops ops = {.mount = ramfs_mount};

static vfs_file_ops file_ops = {
    .open = ramfs_open, .write = ramfs_write, .read = ramfs_read};

static vfs_inode_ops inode_ops = {.evict = ramfs_evict,
                                  .create = ramfs_create,
                                  .lookup = ramfs_lookup,
                                  .unlink = ramfs_unlink,
                                  .rename = ramfs_rename};

static vfs_type ramfs_type = {.name = RAMFS_NAME, .ops = &ops};

void ramfs_init() { register_vfs_type(&ramfs_type); }

static vfs_inode* to_inode(tree_node* node, vfs_super_block* sb) {
    vfs_inode* inode = vfs_icache_get(sb, node->id);
    if (IS_ERROR(inode) || inode->flags & INODE_INITIALIZED)
        return inode;

    inode->size = node->type == FILE ? node->file_data.size : 0;
    // 1 for parent + children count
    inode->links =
        1 + (node->type == DIRECTORY ? node->dir_data.subnodes.size : 0);
    inode->private_data = node;
    inode->ops = &inode_ops;
    inode->file_ops = &file_ops;
    inode->type = node->type;

    return vfs_inode_unlock_new(inode);
}

void ramfs_evict(vfs_inode* inode) { evict_node(inode->private_data); }

u64 ramfs_fill_super(vfs_super_block* sb) {
    vfs_inode* root_inode = to_inode(internal_tree_create(), sb);
    if (IS_ERROR(root_inode))
        return PTR_ERROR(root_inode);

    vfs_dentry* root_dentry = vfs_dentry_create_root(root_inode);
    if (IS_ERROR(root_dentry)) {
        vfs_inode_drop(root_inode);
        return PTR_ERROR(root_dentry);
    }

    vfs_inode_release(root_inode);
    sb->root = root_dentry;

    return 0;
}

vfs_dentry* ramfs_mount(vfs_type* type, device* dev) {
    vfs_super_block* sb = vfs_super_get(type, dev);
    if (IS_ERROR(sb))
        return ERROR_PTR(sb);

    if (sb->flags & SUPER_INITIALIZED) {
        vfs_dentry* root = vfs_dentry_acquire(sb->root);
        vfs_super_release(sb);
        return root;
    }

    u64 res = ramfs_fill_super(sb);
    if (IS_ERROR(res)) {
        vfs_super_unlock_failed(sb);
        vfs_super_release(sb);
        return ERROR_PTR(-ENOMEM);
    }

    vfs_super_unlock_new(sb);
    vfs_dentry* root = vfs_dentry_acquire(sb->root);
    vfs_super_release(sb);
    return root;
}

vfs_dentry* ramfs_lookup(vfs_dentry* parent, string name) {
    tree_node* res = find_subnode(parent->inode->private_data, name);
    if (!res)
        return ERROR_PTR(-ENOENT);

    vfs_inode* inode = to_inode(res, parent->inode->sb);
    if (IS_ERROR(inode))
        return ERROR_PTR(inode);

    vfs_dentry* dentry = vfs_dentry_create(parent, inode, name);
    vfs_inode_release(inode);

    return dentry;
}

vfs_dentry* ramfs_create(vfs_dentry* parent, string name, u64 flags) {
    UNUSED(flags);
    tree_node* child_node = alloc_tree_node(name, FILE);
    if (IS_ERROR(child_node))
        return ERROR_PTR(child_node);

    link_nodes(parent->inode->private_data, child_node);
    return ramfs_lookup(parent, name);
}

u64 ramfs_open(vfs_file* file, u64 mode) {
    UNUSED(file);
    UNUSED(mode);

    return 0;
}

u64 ramfs_write(vfs_file* file, __user void* buf, u64 size) {
    vfs_inode* inode = file->path.dentry->inode;
    tree_node* node = inode->private_data;

    if (file->flags & O_APPEND)
        file->pos = inode->size;

    u64 pos = file->pos;
    if (pos + size > node->file_data.capacity) {
        u64 new_capacity = align_to_upper(pos + size, ALIGNMENT);
        u8* new_buf = krealloc(node->file_data.buf, new_capacity);
        if (!new_buf)
            return -ENOMEM;

        node->file_data.buf = new_buf;
        node->file_data.capacity = new_capacity;
    }

    if (!copy_from_user(node->file_data.buf + pos, buf, size))
        return -EINVAL;

    inode->size = MAX(pos + size, inode->size);
    node->file_data.size = inode->size;
    file->pos += size;
    return size;
}

u64 ramfs_read(vfs_file* file, __user void* buf, u64 size) {
    vfs_inode* inode = file->path.dentry->inode;
    tree_node* node = inode->private_data;

    u64 start = file->pos;
    u64 end = MIN(inode->size, start + size);
    if (end <= start)
        return 0;

    u64 read = end - start;
    if (!copy_to_user(buf, node->file_data.buf + start, read))
        return (u64) -EFAULT;

    file->pos += read;
    return read;
}

u64 ramfs_unlink(vfs_inode* dir, vfs_dentry* child) {
    unlink_nodes(dir->private_data, child->inode->private_data);
    vfs_inode_drop_link(dir);
    vfs_inode_drop_link(child->inode);

    return 0;
}

u64 ramfs_rename(vfs_dentry* old_dir, vfs_dentry* source, vfs_dentry* new_dir,
                 vfs_dentry* target, string name) {

    u64 error = 0;

    tree_node* old_dir_node = old_dir->inode->private_data;
    tree_node* new_dir_node = new_dir->inode->private_data;
    tree_node* source_node = source->inode->private_data;
    tree_node* target_node = target ? target->inode->private_data : NULL;

    // inode private data is guarded by inode->rw_mutex
    error = target_node && target_node->dir_data.subnodes.size != 0 ? -ENOTEMPTY
                                                                    : 0;
    if (IS_ERROR(error))
        return error;

    error = rename_node(source_node, name);
    if (IS_ERROR(error))
        return error;

    unlink_nodes(old_dir_node, source_node);
    link_nodes(new_dir_node, source_node);

    vfs_inode_drop_link(old_dir->inode);
    if (target) {
        unlink_nodes(new_dir_node, target_node);
        vfs_inode_drop_link(target->inode);
    } else
        vfs_inode_add_link(new_dir->inode);

    return 0;
}