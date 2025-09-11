#include "ramfs.h"
#include "../../error/errno.h"
#include "../../error/error.h"
#include "../../lib/string.h"
#include "../dcache.h"

struct vfs_dentry* ramfs_mount(struct vfs_super_block* sb, device* dev);
struct vfs_dentry* lookup(struct vfs_dentry* parent, string name);

typedef struct _tree_node {
    string name;
    u64 id;

    struct _tree_node* parent;
    array_list subnodes;
} tree_node;

tree_node* alloc_tree_node(u64 id, string name) {
    tree_node* node = kmalloc(sizeof(tree_node));
    array_list_init(&node->subnodes, 8);
    node->id = id;
    node->name = strcpy(name);

    return node;
}

void link_nodes(tree_node* parent, tree_node* child) {
    child->parent = parent;
    array_list_add_last(&parent->subnodes, child);
}

tree_node* find_subnode(tree_node* node, string name) {
    if (streq(name, "."))
        return node;

    if (streq(name, ".."))
        return node->parent;

    ARRAY_LIST_FOR_EACH(&node->subnodes, tree_node * subnode) {
        if (streq(subnode->name, name))
            return subnode;
    }
    return NULL;
}

/*
 *                root
 *            /           \
 *            a             b
 *          /   \         /   \
 *         c     d       e      f
 */
static tree_node root = {.id = 0, .name = "[root]"};
static vfs_type_ops ops = {.mount = ramfs_mount, .sync = NULL, .unmount = NULL};
static vfs_inode_ops inode_ops = {.lookup = lookup};
vfs_type ramfs_type = {.name = "ramfs", .ops = &ops};

vfs_inode* to_inode(tree_node* node, struct vfs_super_block* sb) {
    vfs_inode* inode = vfs_icache_get(sb, node->id);

    spin_lock(&inode->lock);
    if (inode->initialised)
        goto out;

    inode->initialised = true;
    inode->private_data = node;
    inode->ops = &inode_ops;
    inode->type = DIRECTORY;

out:
    spin_unlock(&inode->lock);

    return inode;
}

void ramfs_init() {
    register_vfs_type(&ramfs_type);

    array_list_init(&root.subnodes, 8);
    tree_node* a = alloc_tree_node(1, "a");
    tree_node* b = alloc_tree_node(2, "b");
    tree_node* c = alloc_tree_node(3, "c");
    tree_node* d = alloc_tree_node(4, "d");
    tree_node* e = alloc_tree_node(5, "e");
    tree_node* f = alloc_tree_node(6, "f");

    link_nodes(&root, a);
    link_nodes(&root, b);
    link_nodes(a, c);
    link_nodes(a, d);
    link_nodes(b, e);
    link_nodes(b, f);
}

struct vfs_dentry* ramfs_mount(struct vfs_super_block* sb, device* dev) {
    UNUSED(dev);

    vfs_inode* root_inode = to_inode(&root, sb);
    struct vfs_dentry* root_dentry = vfs_dentry_create_root(root_inode);
    vfs_inode_release(root_inode);

    return root_dentry;
}

struct vfs_dentry* lookup(struct vfs_dentry* parent, string name) {
    tree_node* res = find_subnode(parent->inode->private_data, name);
    if (!res)
        return ERROR_PTR(-ENOENT);

    vfs_inode* inode = to_inode(res, parent->inode->sb);
    struct vfs_dentry* dentry = vfs_dentry_create(parent, inode, name);
    vfs_inode_release(inode);

    return dentry;
}