#include "ramfs.h"
#include "../../error/errno.h"
#include "../../error/error.h"
#include "../../lib/flagops.h"
#include "../dcache/dentry.h"
#include "internal_tree.h"

static u64 ramfs_fill_super(struct vfs_super_block* sb, device* dev);
static vfs_dentry* ramfs_mount(struct vfs_type* type, device* dev);
static vfs_dentry* ramfs_lookup(struct vfs_dentry* parent, string name);
static u64 ramfs_rename(vfs_dentry* old_parent_dentry, vfs_dentry* old_dentry,
                        vfs_dentry* new_parent_dentry, vfs_dentry* new_dentry,
                        string name);
static u64 ramfs_unlink(vfs_inode* dir, vfs_dentry* child);

static vfs_type_ops ops = {.fill_super = ramfs_fill_super,
                           .mount = ramfs_mount,
                           .sync = NULL,
                           .unmount = NULL};
static vfs_inode_ops inode_ops = {
    .lookup = ramfs_lookup, .unlink = ramfs_unlink, .rename = ramfs_rename};
static struct vfs_type ramfs_type = {.name = RAMFS_NAME, .ops = &ops};

void ramfs_init() {
    register_vfs_type(&ramfs_type);
    internal_tree_init();
}

vfs_inode* to_inode(tree_node* node, vfs_super_block* sb) {
    vfs_inode* inode = vfs_icache_get(sb, node->id);
    if (IS_ERROR(inode))
        return inode;

    if (inode->flags & INODE_INITIALISED)
        return inode;

    inode->links = 1 + node->subnodes.size;
    inode->private_data = node;
    inode->ops = &inode_ops;
    inode->type = node->type;

    return vfs_inode_unlock_new(inode);
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

    return vfs_dentry_acquire(sb->root);
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
    vfs_inode_drop_link(dir);

    vfs_inode* inode = child->inode;

    unlink_nodes(dir->private_data, inode->private_data);
    vfs_inode_lock(inode);
    vfs_inode_drop_link(inode);
    if (inode->links == 0)
        ATOMIC_SET_FLAGS(inode->flags, INODE_DEAD);
    vfs_inode_lock(inode);

    return 0;
}

u64 ramfs_rename(vfs_dentry* old_parent_dentry, vfs_dentry* old_dentry,
                 vfs_dentry* new_parent_dentry, vfs_dentry* new_dentry,
                 string name) {

    tree_node* old_parent = old_parent_dentry->inode->private_data;
    tree_node* new_parent = new_parent_dentry->inode->private_data;
    tree_node* old_child = old_dentry->inode->private_data;
    tree_node* new_child = new_dentry ? new_dentry->inode->private_data : NULL;

    // lock directory inode so that nobody can modify directory concurrently
    if (new_dentry && new_dentry->inode->type == DIRECTORY) {
        vfs_inode_lock(new_dentry->inode);

        // inode private data is guarded by inode->rw_mutex
        if (new_child->subnodes.size != 0) {
            vfs_inode_unlock(new_dentry->inode);
            return -ENOTEMPTY;
        }
    }

    // fail-safe replacements
    unlink_nodes(old_parent, old_child);
    old_child->name = name;
    link_nodes(new_parent, old_child);

    vfs_inode_drop_link(old_parent_dentry->inode);

    if (new_dentry) {
        unlink_nodes(new_parent, new_child);
        vfs_inode_drop_link(new_dentry->inode);
        if (new_dentry->inode->links == 0)
            ATOMIC_SET_FLAGS(new_dentry->inode->flags, INODE_DEAD);
        vfs_inode_unlock(new_dentry->inode);
    }

    return 0;
}