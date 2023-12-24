#include "signal.h"
#include "../scheduler/scheduler.h"

bool signal_raised(u64 pending_signals, signal sig) {
    return (pending_signals >> sig) & 1;
}

bool signal_allowed(u64 signals_mask, signal sig) {
    return (signals_mask >> sig) & 1;
}

void signal_raise(u64* pending_signals, signal sig) {
    *pending_signals |= 1 << sig;
}

void signal_clear(u64* pending_signals, signal sig) {
    *pending_signals &= ~(1 << sig);
}

void signal_block(u64* signals_mask, signal sig) {
    *signals_mask &= ~(1 << sig);
}

void block_and_clear_all_signals(thread* thrd) {
    thrd->pending_signals = PENDING_SIGNALS_CLEAR;
    thrd->signals_mask = ALL_SIGNALS_BLOCKED;
}

bool signal_thread(thread* thrd, signal sig) {
    bool signal_set = false;
    bool interrupts_enabled = spin_lock_irq_save(&thrd->lock);
    if (signal_allowed(thrd->signals_mask, sig)) {
        signal_raise(&thrd->pending_signals, sig);
        signal_set = true;
    }
    spin_unlock_irq_restore(&thrd->lock, interrupts_enabled);

    return signal_set;
}

extern bool arch_is_userspace_context(struct cpu_context* context);
extern void arch_install_user_signal_handler(struct cpu_context* context,
                                             u64 signal_handler);

void check_pending_signals() {
    thread* current = get_current_thread();
    if (!current || !arch_is_userspace_context(current->context)) {
        return;
    }

    bool interrupts_enabled = spin_lock_irq_save(&current->lock);
    if (current->pending_signals == PENDING_SIGNALS_CLEAR) {
        spin_unlock_irq_restore(&current->lock, interrupts_enabled);
        return;
    }

    if (signal_raised(current->pending_signals, SIGKILL)
        && signal_allowed(current->signals_mask, SIGKILL)) {

        block_and_clear_all_signals(current);

        spin_unlock_irq_restore(&current->lock, interrupts_enabled);
        thread_exit(-1);
    } else if (signal_raised(current->pending_signals, SIGTEST)
               && signal_allowed(current->pending_signals, SIGTEST)) {

        signal_clear(&current->pending_signals, SIGTEST);
        arch_install_user_signal_handler(current->context,
                                         current->signal_handler);
    }
    spin_unlock_irq_restore(&current->lock, interrupts_enabled);
}
