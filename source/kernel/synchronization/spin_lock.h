#ifndef SOS_SPIN_LOCK_H
#define SOS_SPIN_LOCK_H

#include "../lib/types.h"
#include "atomics.h"

typedef volatile u64 lock;

typedef enum { LOCKED = 0, UNLOCKED = 1 } lock_state;

#define SPIN_LOCK_STATIC_INITIALIZER UNLOCKED
#define DECLARE_SPIN_LOCK(name) lock name = UNLOCKED

void init_lock(lock* lock);
bool try_lock(lock* lock);
void spin_lock(lock* lock);
void spin_unlock(lock* lock);

void spin_lock_irq(lock* lock);
void spin_unlock_irq(lock* lock);

bool spin_lock_irq_save(lock* lock);
void spin_unlock_irq_restore(lock* lock, bool interrupts_enabled);

#endif
