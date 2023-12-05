#ifndef SOS_RW_SPIN_LOCK_H
#define SOS_RW_SPIN_LOCK_H

#include "../lib/types.h"
#include "spin_lock.h"

typedef struct {
    i64 readers;
    bool writer_involved;
    lock lock;
} rw_spin_lock;

void init_rw_lock(rw_spin_lock* lock);

void rw_spin_lock_write(rw_spin_lock* lock);
void rw_spin_unlock_write(rw_spin_lock* lock);

void rw_spin_lock_write_irq(rw_spin_lock* lock);
void rw_spin_unlock_write_irq(rw_spin_lock* lock);

void rw_spin_lock_read(rw_spin_lock* lock);
void rw_spin_unlock_read(rw_spin_lock* lock);

void rw_spin_lock_read_irq(rw_spin_lock* lock);
void rw_spin_unlock_read_irq(rw_spin_lock* lock);

#endif // SOS_RW_SPIN_LOCK_H
