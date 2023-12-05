#ifndef SOS_CON_VAR_H
#define SOS_CON_VAR_H

#include "../lib/container/linked_list/linked_list.h"
#include "mutex.h"
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
    linked_list* wait_list;
} con_var;

void con_var_init(con_var* var);

void con_var_wait(con_var* var, lock* lock);
bool con_var_wait_irq_save(con_var* var, lock* lock, bool interrupts_enabled);

void con_var_signal(con_var* var);
void con_var_broadcast(con_var* var);

#endif // SOS_CON_VAR_H