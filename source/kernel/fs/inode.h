#ifndef SOS_INODE_H
#define SOS_INODE_H

#include "vfs.h"

typedef struct {
    u64 id;

    vfs* fs;
    vfs_inode_type type;
    vfs_inode_ops* ops;

    lock lock; // guards all fields below
    ref_count refc;
    void* private_data;
} vfs_inode;

void vfs_inode_acquire(vfs_inode* inode) {
    bool interrupts_enabled = spin_lock_irq_save(&inode->lock);
    ref_acquire(&inode->refc);
    spin_unlock_irq_restore(&inode->lock, interrupts_enabled);
}

void vfs_inode_release(vfs_inode* inode) {
    bool interrupts_enabled = spin_lock_irq_save(&inode->lock);
    ref_release(&inode->refc);
    spin_unlock_irq_restore(&inode->lock, interrupts_enabled);
}

#endif // SOS_INODE_H
