#include "mutex.h"

void mutex_init(mutex* mutex) {
    init_lock(&mutex->lock);
    con_var_init(&mutex->cvar);
    mutex->locked = false;
}

void mutex_lock(mutex* mutex) {
    spin_lock(&mutex->lock);
    while (mutex->locked) {
        con_var_wait(&mutex->cvar, &mutex->lock);
    }
    mutex->locked = true;
    spin_unlock(&mutex->lock);
}

void mutex_unlock(mutex* mutex) {
    spin_lock(&mutex->lock);
    mutex->locked = false;
    con_var_broadcast(&mutex->cvar);
    spin_unlock(&mutex->lock);
}