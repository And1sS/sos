#ifndef SOS_MUTEX_H
#define SOS_MUTEX_H

#include "con_var.h"
#include "spin_lock.h"

/*
 * Basic mutual exclusion primitive, effectively - binary semaphore. Should be
 * used to coordinate access to huge critical sections. Can be used inside any
 * thread that needs synchronization, but not inside irq handlers, since this
 * will lead to deadlock.
 */

typedef struct {
    lock lock;
    bool locked;
    con_var cvar;
} mutex;

#define MUTEX_STATIC_INITIALIZER                                               \
    {                                                                          \
        .lock = SPIN_LOCK_STATIC_INITIALIZER, .locked = false,                 \
        .cvar = CON_VAR_STATIC_INITIALIZER                                     \
    }

#define DECLARE_MUTEX(name) mutex name = MUTEX_STATIC_INITIALIZER

void mutex_init(mutex* mutex);

void mutex_lock(mutex* mutex);
void mutex_unlock(mutex* mutex);

#endif // SOS_MUTEX_H
