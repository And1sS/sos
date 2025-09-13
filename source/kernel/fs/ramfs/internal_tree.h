#ifndef SOS_INTERNAL_TREE_H
#define SOS_INTERNAL_TREE_H

#include "../../lib/container/array_list/array_list.h"
#include "../../lib/types.h"

/*
 *                root
 *            /           \
 *            a             b
 *          /   \         /   \
 *         c     d       e      f
 */
typedef struct _tree_node {
    string name;
    u64 id;

    struct _tree_node* parent;
    array_list subnodes;
} tree_node;

void internal_tree_init();

tree_node* get_root();
tree_node* find_subnode(tree_node* node, string name);

#endif // SOS_INTERNAL_TREE_H
