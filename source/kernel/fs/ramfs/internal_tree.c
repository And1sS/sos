#include "internal_tree.h"
#include "../../lib/string.h"

static tree_node root = {.name = "[root]",
                         .id = 0,
                         .type = DIRECTORY,
                         .subnodes = LINKED_LIST_STATIC_INITIALIZER};

static tree_node* alloc_tree_node(u64 id, string name, vfs_inode_type type);

/*
 *                root
 *            /           \
 *           a             b
 *          /             /
 *         c             e
 *        /
 *       d
 *      /
 *     f
 */
void internal_tree_init() {
    tree_node* a = alloc_tree_node(1, "a", DIRECTORY);
    tree_node* b = alloc_tree_node(2, "b", DIRECTORY);
    tree_node* c = alloc_tree_node(3, "c", DIRECTORY);
    tree_node* d = alloc_tree_node(4, "d", DIRECTORY);
    tree_node* e = alloc_tree_node(5, "e", FILE);
    tree_node* f = alloc_tree_node(6, "f", FILE);

    link_nodes(&root, a);
    link_nodes(&root, b);
    link_nodes(a, c);
    link_nodes(c, d);
    link_nodes(d, f);
    link_nodes(b, e);
}

static tree_node* alloc_tree_node(u64 id, string name, vfs_inode_type type) {
    tree_node* node = kmalloc(sizeof(tree_node));
    node->id = id;
    node->name = strcpy(name);
    node->type = type;
    node->subnodes = LINKED_LIST_STATIC_INITIALIZER;
    node->self_node = LINKED_LIST_NODE_OF(node);

    return node;
}

void link_nodes(tree_node* parent, tree_node* child) {
    child->parent = parent;
    linked_list_add_last_node(&parent->subnodes, &child->self_node);
}

void unlink_nodes(tree_node* parent, tree_node* child) {
    linked_list_remove_node(&parent->subnodes, &child->self_node);
    child->parent = NULL;
}

tree_node* get_root() { return &root; }

tree_node* find_subnode(tree_node* node, string name) {
    linked_list_node* result =
        LINKED_LIST_FIND(&node->subnodes, subnode,
                         streq(((tree_node*) subnode->value)->name, name));

    return result ? result->value : NULL;
}

static void _pt(tree_node* node) {
    LINKED_LIST_FOR_EACH(&node->subnodes, iter) {
        print(node->name);
        print(" -> ");
        print(((tree_node*) iter->value)->name);
        print("; ");
        _pt(iter->value);
    }
}

void print_tree() {
    println("");
    println("-------------- ramfs --------------");
    _pt(&root);
    println("");
    println("-----------------------------------");
}