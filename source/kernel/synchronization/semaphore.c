#include "semaphore.h"

void semaphore_init(semaphore* sema, u64 max) {
    init_lock(&sema->lock);
    sema->awailable = max;
    con_var_init(&sema->cvar);
}

void semaphore_down(semaphore* sema) {
    spin_lock(&sema->lock);
    while (sema->awailable == 0) {
        con_var_wait(&sema->cvar, &sema->lock);
    }

    sema->awailable--;
    spin_unlock(&sema->lock);
}

void semaphore_up(semaphore* sema) {
    spin_lock(&sema->lock);
    sema->awailable++;
    con_var_signal(&sema->cvar);
    spin_unlock(&sema->lock);
}
