#include "internal_tree.h"
#include "../../lib/string.h"

static tree_node root = {.id = 0, .type = DIRECTORY};

static tree_node* alloc_tree_node(u64 id, string name, vfs_inode_type type);
static void link_nodes(tree_node* parent, tree_node* child);

/*
 *                root
 *            /           \
 *            a             b
 *          /   \         /   \
 *         c     d       e      f
 */
void internal_tree_init() {
    array_list_init(&root.subnodes, 8);
    tree_node* a = alloc_tree_node(1, "a", DIRECTORY);
    tree_node* b = alloc_tree_node(2, "b", DIRECTORY);
    tree_node* c = alloc_tree_node(3, "c", FILE);
    tree_node* d = alloc_tree_node(4, "d", FILE);
    tree_node* e = alloc_tree_node(5, "e", FILE);
    tree_node* f = alloc_tree_node(6, "f", FILE);

    link_nodes(&root, a);
    link_nodes(&root, b);
    link_nodes(a, c);
    link_nodes(a, d);
    link_nodes(b, e);
    link_nodes(b, f);
}

static tree_node* alloc_tree_node(u64 id, string name, vfs_inode_type type) {
    tree_node* node = kmalloc(sizeof(tree_node));
    array_list_init(&node->subnodes, 8);
    node->id = id;
    node->name = strcpy(name);
    node->type = type;

    return node;
}

static void link_nodes(tree_node* parent, tree_node* child) {
    child->parent = parent;
    array_list_add_last(&parent->subnodes, child);
}

void unlink_nodes(tree_node* parent, tree_node* child) {
    array_list_remove(&parent->subnodes, child);
    child->parent = NULL;
}

tree_node* get_root() { return &root; }

tree_node* find_subnode(tree_node* node, string name) {
    ARRAY_LIST_FOR_EACH(&node->subnodes, tree_node * subnode) {
        if (streq(subnode->name, name))
            return subnode;
    }
    return NULL;
}