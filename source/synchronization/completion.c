#include "completion.h"

void completion_init(completion* completion) {
    init_lock(&completion->lock);
    con_var_init(&completion->cvar);
    completion->completed = false;
}

void completion_wait(completion* completion) {
    spin_lock(&completion->lock);
    while (!completion->completed) {
        con_var_wait(&completion->cvar, &completion->lock);
    }
    spin_unlock(&completion->lock);
}

void completion_wait_irq(completion* completion) {
    bool interrupts_enabled = spin_lock_irq_save(&completion->lock);
    while (!completion->completed) {
        interrupts_enabled = con_var_wait_irq_save(
            &completion->cvar, &completion->lock, interrupts_enabled);
    }
    spin_unlock_irq_restore(&completion->lock, interrupts_enabled);
}

void completion_complete(completion* completion) {
    spin_lock(&completion->lock);
    completion->completed = true;
    con_var_broadcast(&completion->cvar);
    spin_unlock(&completion->lock);
}

void completion_complete_irq(completion* completion) {
    bool interrupts_enabled = spin_lock_irq_save(&completion->lock);
    completion->completed = true;
    con_var_broadcast(&completion->cvar);
    spin_unlock_irq_restore(&completion->lock, interrupts_enabled);
}