#include "internal_tree.h"
#include "../../error/errno.h"
#include "../../error/error.h"
#include "../../lib/string.h"

static volatile u64 id_gen = 0;

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
tree_node* internal_tree_create() {
    tree_node* root = alloc_tree_node("[root]", DIRECTORY);
    tree_node* a = alloc_tree_node("a", DIRECTORY);
    tree_node* b = alloc_tree_node("b", DIRECTORY);
    tree_node* c = alloc_tree_node("c", DIRECTORY);
    tree_node* d = alloc_tree_node("d", DIRECTORY);
    tree_node* e = alloc_tree_node("e", FILE);
    tree_node* f = alloc_tree_node("f", FILE);

    link_nodes(root, a);
    link_nodes(root, b);
    link_nodes(a, c);
    link_nodes(c, d);
    link_nodes(d, f);
    link_nodes(b, e);

    return root;
}

tree_node* alloc_tree_node(string name, vfs_inode_type type) {
    if (type != DIRECTORY && type != FILE)
        return ERROR_PTR(-EPERM);

    tree_node* node = kmalloc(sizeof(tree_node));
    if (!node)
        return ERROR_PTR(-ENOMEM);

    memset(node, 0, sizeof(tree_node));

    node->id = atomic_increment_and_get(&id_gen);
    node->name = strcpy(name);
    if (!node->name)
        goto error_out;

    node->type = type;
    node->self_node = LINKED_LIST_NODE_OF(node);

    if (node->type == DIRECTORY) {
        node->dir_data.subnodes = LINKED_LIST_STATIC_INITIALIZER;
    } else if (node->type == FILE) {
        node->file_data.buf = kmalloc(ALIGNMENT);
        if (!node->file_data.buf)
            goto error_out;

        node->file_data.capacity = ALIGNMENT;
    }

    return node;

error_out:
    if (node->name)
        strfree(node->name);

    kfree(node);
    return ERROR_PTR(-ENOMEM);
}

void evict_node(tree_node* node) {
    strfree(node->name);
    if (node->type == FILE)
        kfree(node->file_data.buf);

    kfree(node);
}

void link_nodes(tree_node* parent, tree_node* child) {
    child->parent = parent;
    linked_list_add_last_node(&parent->dir_data.subnodes, &child->self_node);
}

void unlink_nodes(tree_node* parent, tree_node* child) {
    linked_list_remove_node(&parent->dir_data.subnodes, &child->self_node);
    child->parent = NULL;
}

u64 rename_node(tree_node* node, string name) {
    string name_copy = strcpy(name);
    if (!name_copy)
        return -ENOMEM;

    strfree(node->name);
    node->name = name_copy;
    return 0;
}

tree_node* find_subnode(tree_node* node, string name) {
    linked_list_node* result =
        LINKED_LIST_FIND(&node->dir_data.subnodes, subnode,
                         streq(((tree_node*) subnode->value)->name, name));

    return result ? result->value : NULL;
}