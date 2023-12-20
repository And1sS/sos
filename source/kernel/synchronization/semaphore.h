#ifndef SOS_SEMAPHORE_H
#define SOS_SEMAPHORE_H

#include "../lib/types.h"
#include "con_var.h"
#include "spin_lock.h"

/*
 * Semaphore primitive - should be used to limit access to huge critical
 * sections. Can be used inside any thread that needs synchronization, but not
 * inside irq handlers, since this will lead to deadlock.
 */

typedef struct {
    lock lock;
    u64 awailable;
    con_var cvar;
} semaphore;

#define SEMAPHORE_STATIC_INITIALIZER(max)                                      \
    {                                                                          \
        .lock = SPIN_LOCK_STATIC_INITIALIZER, .awailable = max,                \
        .cvar = CON_VAR_STATIC_INITIALIZER                                     \
    }

#define DECLARE_SEMAPHORE(name, max)                                           \
    semaphore name = SEMAPHORE_STATIC_INITIALIZER(max)

void semaphore_init(semaphore* sema, u64 max);

void semaphore_down(semaphore* sema);
void semaphore_up(semaphore* sema);

#endif // SOS_SEMAPHORE_H
