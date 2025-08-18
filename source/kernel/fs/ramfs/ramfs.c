#include "ramfs.h"
#include "../../error/errno.h"
#include "../../lib/container/array_list/array_list.h"
#include "../../lib/string.h"
#include "../../memory/heap/kheap.h"
#include "../icache.h"
#include "../inode.h"
#include "../vfs.h"

struct vfs_inode* ramfs_mount(struct vfs* fs, device* dev);
u64 lookup(struct vfs_inode* dir, string name, struct vfs_inode** result);

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
static vfs_ops ops = {.mount = ramfs_mount, .sync = NULL, .unmount = NULL};
static vfs_inode_ops inode_ops = {.lookup = lookup};
static vfs_type type = {.name = "ramfs", .ops = &ops};
static vfs* this;

struct vfs_inode* to_inode(tree_node* node) {
    vfs_inode* inode = icache_get(this, 0);
    inode->private_data = node;
    inode->id = node->id;
    inode->ops = &inode_ops;
    inode->type = DIRECTORY;
    inode->fs = this;
    inode->lock = SPIN_LOCK_STATIC_INITIALIZER;
    inode->refc = REF_COUNT_STATIC_INITIALIZER;
    ref_acquire(&inode->refc);

    return (struct vfs_inode*) inode;
}

void ramfs_init() {
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

    this = kmalloc(sizeof(vfs));
    this->root = to_inode(&root);
    this->id = 0;
    this->type = &type;
}

struct vfs_inode* ramfs_mount(struct vfs* fs, device* dev) {
    UNUSED(fs);
    UNUSED(dev);

    return to_inode(&root);
}

u64 lookup(struct vfs_inode* dir, string name, struct vfs_inode** result) {
    *result = NULL;
    tree_node* node = ((vfs_inode*) dir)->private_data;
    tree_node* res = find_subnode(node, name);
    if (!res)
        return -ENOENT;

    *result = to_inode(res);
    return 0;
}