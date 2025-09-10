#include "ref_count.h"

void ref_acquire(ref_count* refc) { atomic_increment(&refc->count); }

void ref_release(ref_count* refc) {
    if (refc->count == 0) {
        panic("ref_count is less than 0");
    }

    refc->count--;
    if (refc->count == 0) {
        con_var_broadcast(&refc->empty_cvar);
    }
}
