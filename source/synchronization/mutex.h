#ifndef SOS_MUTEX_H
#define SOS_MUTEX_H

#include "con_var.h"
#include "spin_lock.h"

/*
 * Basic mutual exclusion primitive, should be used to coordinate access to huge
 * critical sections. Can be used inside any thread that needs
 * synchronization, but not inside irq handlers, since this will lead to
 * deadlock.
 */

typedef struct {
    lock lock;
    bool locked;
    con_var cvar;
} mutex;

void mutex_init(mutex* mutex);
void mutex_destroy(mutex* mutex);

void mutex_lock(mutex* mutex);
void mutex_unlock(mutex* mutex);

#endif // SOS_MUTEX_H
