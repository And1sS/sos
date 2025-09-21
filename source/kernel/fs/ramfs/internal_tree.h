#ifndef SOS_INTERNAL_TREE_H
#define SOS_INTERNAL_TREE_H

#include "../../lib/container/array_list/array_list.h"
#include "../../lib/types.h"
#include "../vfs.h"

typedef struct _tree_node {
    string name;
    u64 id;

    vfs_inode_type type;

    struct _tree_node* parent;
    array_list subnodes;
} tree_node;

void internal_tree_init();

tree_node* get_root();
tree_node* find_subnode(tree_node* node, string name);

void unlink_nodes(tree_node* parent, tree_node* child);

#endif // SOS_INTERNAL_TREE_H
