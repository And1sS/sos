#include "ramfs.h"
#include "../../error/errno.h"
#include "../../error/error.h"
#include "../dentry.h"
#include "internal_tree.h"

static u64 ramfs_fill_super(struct vfs_super_block* sb, device* dev);
static struct vfs_dentry* ramfs_mount(struct vfs_type* type, device* dev);
static struct vfs_dentry* ramfs_lookup(struct vfs_dentry* parent, string name);
static u64 ramfs_unlink(vfs_inode* dir, vfs_dentry* child);

static vfs_type_ops ops = {.fill_super = ramfs_fill_super,
                           .mount = ramfs_mount,
                           .sync = NULL,
                           .unmount = NULL};
static vfs_inode_ops inode_ops = {.lookup = ramfs_lookup,
                                  .unlink = ramfs_unlink};
static struct vfs_type ramfs_type = {.name = RAMFS_NAME, .ops = &ops};

void ramfs_init() {
    register_vfs_type(&ramfs_type);
    internal_tree_init();
}

vfs_inode* to_inode(tree_node* node, struct vfs_super_block* sb) {
    vfs_inode* inode = vfs_icache_get(sb, node->id);
    if (IS_ERROR(inode))
        return inode;

    spin_lock(&inode->lock);
    if (inode->initialised)
        goto out;

    inode->initialised = true;
    inode->links = 1;
    inode->private_data = node;
    inode->ops = &inode_ops;
    inode->type = node->type;

out:
    spin_unlock(&inode->lock);

    return inode;
}

u64 ramfs_fill_super(vfs_super_block* sb, device* dev) {
    vfs_inode* root_inode = to_inode(get_root(), sb);
    if (IS_ERROR(root_inode))
        return PTR_ERROR(root_inode);

    vfs_dentry* root_dentry = vfs_dentry_create_root(root_inode);
    if (IS_ERROR(root_dentry)) {
        vfs_inode_drop(root_inode);
        return PTR_ERROR(root_dentry);
    }

    vfs_inode_release(root_inode);

    sb->root = root_dentry;
    sb->device = dev;

    return 0;
}

vfs_dentry* ramfs_mount(vfs_type* type, device* dev) {
    vfs_super_block* sb = vfs_super_get(type, dev);
    if (IS_ERROR(sb))
        return ERROR_PTR(sb);

    vfs_dentry_acquire(sb->root);
    return sb->root;
}

vfs_dentry* ramfs_lookup(vfs_dentry* parent, string name) {
    tree_node* res = find_subnode(parent->inode->private_data, name);
    if (!res)
        return ERROR_PTR(-ENOENT);

    vfs_inode* inode = to_inode(res, parent->inode->sb);
    vfs_dentry* dentry = vfs_dentry_create(parent, inode, name);
    vfs_inode_release(inode);

    return dentry;
}

u64 ramfs_unlink(vfs_inode* dir, vfs_dentry* child) {
    unlink_nodes(dir->private_data, child->inode->private_data);
    return 0;
}