#include "rw_spin_lock.h"
#include "../idle.h"

void init_rw_lock(rw_spin_lock* lock) {
    lock->writer_involved = false;
    lock->readers = 0;
    init_lock(&lock->lock);
}

void rw_spin_lock_write(rw_spin_lock* lock) {
    spin_lock(&lock->lock);

    while (lock->writer_involved) {
        spin_unlock(&lock->lock);
        pause();
        spin_lock(&lock->lock);
    }
    lock->writer_involved = true;

    while (lock->readers != 0) {
        spin_unlock(&lock->lock);
        pause();
        spin_lock(&lock->lock);
    }
    spin_unlock(&lock->lock);
}

void rw_spin_unlock_write(rw_spin_lock* lock) {
    spin_lock(&lock->lock);
    lock->writer_involved = false;
    spin_unlock(&lock->lock);
}

void rw_spin_lock_write_irq(rw_spin_lock* lock) {
    bool interrupts_enabled = spin_lock_irq_save(&lock->lock);

    while (lock->writer_involved) {
        spin_unlock_irq_restore(&lock->lock, interrupts_enabled);
        pause();
        interrupts_enabled = spin_lock_irq_save(&lock->lock);
    }
    lock->writer_involved = true;

    while (lock->readers != 0) {
        spin_unlock_irq_restore(&lock->lock, interrupts_enabled);
        pause();
        interrupts_enabled = spin_lock_irq_save(&lock->lock);
    }
    spin_unlock_irq_restore(&lock->lock, interrupts_enabled);
}

void rw_spin_unlock_write_irq(rw_spin_lock* lock) {
    bool interrupts_enabled = spin_lock_irq_save(&lock->lock);
    lock->writer_involved = false;
    spin_unlock_irq_restore(&lock->lock, interrupts_enabled);
}

void rw_spin_lock_read(rw_spin_lock* lock) {
    spin_lock(&lock->lock);

    while (lock->writer_involved) {
        spin_unlock(&lock->lock);
        pause();
        spin_lock(&lock->lock);
    }

    lock->readers++;
    spin_unlock(&lock->lock);
}

void rw_spin_unlock_read(rw_spin_lock* lock) {
    spin_lock(&lock->lock);
    lock->readers--;
    spin_unlock(&lock->lock);
}

void rw_spin_lock_read_irq(rw_spin_lock* lock) {
    bool interrupts_enabled = spin_lock_irq_save(&lock->lock);

    while (lock->writer_involved) {
        spin_unlock_irq_restore(&lock->lock, interrupts_enabled);
        pause();
        interrupts_enabled = spin_lock_irq_save(&lock->lock);
    }

    lock->readers++;
    spin_unlock_irq_restore(&lock->lock, interrupts_enabled);
}

void rw_spin_unlock_read_irq(rw_spin_lock* lock) {
    bool interrupts_enabled = spin_lock_irq_save(&lock->lock);
    lock->readers--;
    spin_unlock_irq_restore(&lock->lock, interrupts_enabled);
}
