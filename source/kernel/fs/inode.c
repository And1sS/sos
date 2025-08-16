#include "inode.h"

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