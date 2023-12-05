#include "spin_lock.h"
#include "../idle.h"
#include "../interrupts/irq.h"

void init_lock(lock* lock) { atomic_set(lock, 1); }

bool try_lock(lock* lock) { return atomic_exchange(lock, 0) == 1; }

void spin_lock(lock* lock) {
    do {
        pause();
    } while (!try_lock(lock));
}

void spin_unlock(lock* lock) { atomic_set(lock, 1); }

void spin_lock_irq(lock* lock) {
    local_irq_disable();
    spin_lock(lock);
}

void spin_unlock_irq(lock* lock) {
    spin_unlock(lock);
    local_irq_enable();
}

bool spin_lock_irq_save(lock* lock) {
    bool before = local_irq_enabled();
    spin_lock_irq(lock);

    return before;
}

void spin_unlock_irq_restore(lock* lock, bool interrupts_enabled) {
    spin_unlock(lock);
    if (interrupts_enabled) {
        local_irq_enable();
    }
}
