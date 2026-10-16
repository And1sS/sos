#ifndef SOS_CON_VAR_H
#define SOS_CON_VAR_H

#include "../lib/container/queue/queue.h"
#include "spin_lock.h"

/*
 * Basic conditional variable primitive, should be used to coordinate access to
 * huge critical sections. Can be used inside any thread that needs
 * synchronization, but wait functions should not be used inside irq handlers.
 *
 * All con_var functions(except initialization) should be called with same
 * guarding lock held.
 */

typedef struct {
    queue wait_queue;
} con_var;

#define CON_VAR_STATIC_INITIALIZER                                             \
    (con_var) { .wait_queue = QUEUE_STATIC_INITIALIZER }

#define DECLARE_CON_VAR(name) con_var name = CON_VAR_STATIC_INITIALIZER;

#define CON_VAR_WAIT_FOR_IRQ(cvar, lock, flags, cond)                          \
    do {                                                                       \
        while (!(cond)) {                                                      \
            (flags) = con_var_wait_irq_save((cvar), (lock), (flags));          \
        }                                                                      \
    } while (0)

#define CON_VAR_WAIT_FOR(cvar, lock, cond)                                     \
    do {                                                                       \
        while (!(cond))                                                        \
            con_var_wait((cvar), (lock));                                      \
    } while (0)

void con_var_init(con_var* var);

void con_var_wait(con_var* var, lock* lock);
bool con_var_wait_irq_save(con_var* var, lock* lock, bool interrupts_enabled);

void con_var_signal(con_var* var);
void con_var_broadcast(con_var* var);

#endif // SOS_CON_VAR_H
