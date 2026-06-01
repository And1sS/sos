#ifndef SOS_INTERNAL_TREE_H
#define SOS_INTERNAL_TREE_H

#include "../../lib/container/array_list/array_list.h"
#include "../../lib/types.h"
#include "../inode.h"
#include "../vfs.h"

#define ALIGNMENT 4096

typedef struct tree_node {
    string name;
    u64 id;

    vfs_inode_type type;
    union {
        struct {
            u8* buf;
            u64 size;
            u64 capacity;
        } file_data;

        struct {
            linked_list subnodes;
        } dir_data;
    };

    struct tree_node* parent;
    linked_list_node self_node;
} tree_node;

tree_node* internal_tree_create();

tree_node* alloc_tree_node(string name, vfs_inode_type type);

tree_node* get_root();
tree_node* find_subnode(tree_node* node, string name);

u64 rename_node(tree_node* node, string name);

void evict_node(tree_node* node);

void unlink_nodes(tree_node* parent, tree_node* child);
void link_nodes(tree_node* parent, tree_node* child);

#endif // SOS_INTERNAL_TREE_H
