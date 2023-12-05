#ifndef SOS_COMPLETION_H
#define SOS_COMPLETION_H

#include "con_var.h"
#include "spin_lock.h"

/*
 * Completion primitive - blocking, should be used to coordinate access to
 * huge critical sections. Can be used inside any routine that needs
 * to track completion of something, but wait function should not be used inside
 * irq handler because of its blocking nature. Functions with _irq part should
 * be used when primitive can be used inside irq handlers.
 */

typedef struct {
    lock lock;
    con_var cvar;
    bool completed;
} completion;

#define COMPLETION_STATIC_INITIALIZER                                          \
    {                                                                          \
        .lock = SPIN_LOCK_STATIC_INITIALIZER,                                  \
        .cvar = CON_VAR_STATIC_INITIALIZER, .completed = false                 \
    }

#define DECLARE_COMPLETION(name) completion name = COMPLETION_STATIC_INITIALIZER

void completion_init(completion* completion);

void completion_wait(completion* completion);
void completion_wait_irq(completion* completion);

void completion_complete(completion* completion);
void completion_complete_irq(completion* completion);

#endif // SOS_COMPLETION_H
