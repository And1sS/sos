#ifndef SOS_REF_COUNT_H
#define SOS_REF_COUNT_H

#include "../../lib/panic.h"
#include "../../synchronization/con_var.h"
#include "../types.h"

typedef struct {
    u64 count;
    con_var empty_cvar;
} ref_count;

#define REF_COUNT_STATIC_INITIALIZER                                           \
    { .count = 0, .empty_cvar = CON_VAR_STATIC_INITIALIZER }

void ref_acquire(ref_count* refc);
void ref_release(ref_count* refc);

#endif // SOS_REF_COUNT_H
