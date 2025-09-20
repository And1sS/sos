#ifndef SOS_RW_MUTEX_H
#define SOS_RW_MUTEX_H

#include "mutex.h"

/*
 * Same as mutex, but allows multiple simultaneous readers or single writer
 */
typedef struct {
    lock lock;
    i64 readers;
    bool writer_involved;
    con_var cvar;
} rw_mutex;

#define RW_MUTEX_STATIC_INITIALIZER                                            \
    (rw_mutex) {                                                               \
        .lock = SPIN_LOCK_STATIC_INITIALIZER, .readers = 0,                    \
        .writer_involved = 0, .cvar = CON_VAR_STATIC_INITIALIZER               \
    }

void rw_mutex_lock_write(rw_mutex* mutex);
void rw_mutex_unlock_write(rw_mutex* mutex);

void rw_mutex_lock_read(rw_mutex* mutex);
void rw_mutex_unlock_read(rw_mutex* mutex);

#endif // SOS_RW_MUTEX_H
