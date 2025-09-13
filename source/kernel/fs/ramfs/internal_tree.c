#include "internal_tree.h"
#include "../../lib/string.h"
#include "../../memory/heap/kheap.h"

static tree_node root = {.id = 0, .name = "[root]"};

static tree_node* alloc_tree_node(u64 id, string name);
static void link_nodes(tree_node* parent, tree_node* child);

void internal_tree_init() {
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

static tree_node* alloc_tree_node(u64 id, string name) {
    tree_node* node = kmalloc(sizeof(tree_node));
    array_list_init(&node->subnodes, 8);
    node->id = id;
    node->name = strcpy(name);

    return node;
}

static void link_nodes(tree_node* parent, tree_node* child) {
    child->parent = parent;
    array_list_add_last(&parent->subnodes, child);
}

tree_node* get_root() { return &root; }

tree_node* find_subnode(tree_node* node, string name) {
    ARRAY_LIST_FOR_EACH(&node->subnodes, tree_node * subnode) {
        if (streq(subnode->name, name))
            return subnode;
    }
    return NULL;
}