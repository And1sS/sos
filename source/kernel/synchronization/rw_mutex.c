#include "rw_mutex.h"

void rw_mutex_lock_write(rw_mutex* mutex) {
    spin_lock(&mutex->lock);
    while (mutex->writer_involved)
        con_var_wait(&mutex->cvar, &mutex->lock);

    mutex->writer_involved = true;
    while (mutex->readers != 0)
        con_var_wait(&mutex->cvar, &mutex->lock);

    spin_unlock(&mutex->lock);
}

void rw_mutex_unlock_write(rw_mutex* mutex) {
    spin_lock(&mutex->lock);
    mutex->writer_involved = false;
    con_var_broadcast(&mutex->cvar);
    spin_unlock(&mutex->lock);
}

void rw_mutex_lock_read(rw_mutex* mutex) {
    spin_lock(&mutex->lock);
    while (mutex->writer_involved)
        con_var_wait(&mutex->cvar, &mutex->lock);

    mutex->readers++;
    spin_unlock(&mutex->lock);
}

void rw_mutex_unlock_read(rw_mutex* mutex) {
    spin_lock(&mutex->lock);
    mutex->readers--;
    if (mutex->readers == 0)
        con_var_broadcast(&mutex->cvar);
    spin_unlock(&mutex->lock);
}
