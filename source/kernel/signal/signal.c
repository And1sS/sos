#include "signal.h"
#include "../arch/common/context.h"
#include "../arch/common/signal.h"
#include "../lib/kprint.h"
#include "../lib/math.h"
#include "../scheduler/scheduler.h"

signal_disposition default_dispositions[SIGNALS_COUNT + 1] = {
    CORE_DUMP,
    [SIGHUP] = TERMINATE,
    [SIGINT] = TERMINATE,
    [SIGQUIT] = CORE_DUMP,
    [SIGILL] = CORE_DUMP,
    CORE_DUMP, // Signal not supported for now
    [SIGABRT] = CORE_DUMP,
    CORE_DUMP, // Signal not supported for now
    CORE_DUMP, // Signal not supported for now
    [SIGKILL] = TERMINATE,
    CORE_DUMP, // Signal not supported for now
    [SIGSEGV] = CORE_DUMP,
    CORE_DUMP, // Signal not supported for now
    CORE_DUMP, // Signal not supported for now
    [SIGTERM] = TERMINATE,
    [SIGTERM + 1 ... SIGNALS_COUNT] = FALLBACK_TO_DEFAULT};

bool signal_raised(sigpending pending_signals, signal sig) {
    return (pending_signals >> sig) & 1;
}

bool signal_any_raised(sigpending pending_signals) {
    return pending_signals != PENDING_SIGNALS_CLEAR;
}

bool signal_allowed(sigmask signals_mask, signal sig) {
    return (signals_mask >> sig) & 1;
}

signal signal_first_raised(sigpending pending_signals) {
    return lsb_u64(pending_signals);
}

void signal_raise(sigpending* pending_signals, signal sig) {
    *pending_signals |= 1 << sig;
}

void signal_clear(sigpending* pending_signals, signal sig) {
    *pending_signals &= ~(1 << sig);
}

void signal_block(sigmask* signals_mask, signal sig) {
    *signals_mask &= ~(1 << sig);
}

bool signal_set_action(siginfo* signal_info, signal sig, sigaction action) {
    if (sig == SIGKILL)
        return false;

    signal_info->signal_actions[sig] = action;
    return true;
}

static void block_and_clear_all_signals(thread* thrd) {
    thrd->signal_info.pending_signals = PENDING_SIGNALS_CLEAR;
    thrd->signal_info.signals_mask = ALL_SIGNALS_BLOCKED;
}

void check_pending_signals() {
    thread* current = get_current_thread();
    if (!current || !arch_is_userspace_context(current->context)) {
        return;
    }

    bool interrupts_enabled = spin_lock_irq_save(&current->lock);
    siginfo* signal_info = &current->signal_info;
    if (!signal_any_raised(signal_info->pending_signals)) {
        spin_unlock_irq_restore(&current->lock, interrupts_enabled);
        return;
    }

    signal raised = signal_first_raised(signal_info->pending_signals);
    print("signal raised: ");
    print_u64(raised);
    println("");

    sigaction action = signal_info->signal_actions[raised];
    signal_handler* handler = action.handler;

    if (handler) {
        signal_clear(&signal_info->pending_signals, raised);
        arch_enter_signal_handler(current->context, handler);
    } else {
        signal_disposition disposition =
            action.disposition != FALLBACK_TO_DEFAULT
                ? action.disposition
                : default_dispositions[raised];

        switch (disposition) {
        case FALLBACK_TO_DEFAULT:
            panic("Signal handling error, resolved disposition is fallback");
            return;

        case TERMINATE:
        case CORE_DUMP:
            println("Core dumped: ");
            arch_print_cpu_context(current->context);
            block_and_clear_all_signals(current);
            spin_unlock_irq_restore(&current->lock, interrupts_enabled);
            thread_exit(-1);
            return;

        case STOP:
        case IGNORE:
        case CONTINUE:
            break;
        }
    }
    spin_unlock_irq_restore(&current->lock, interrupts_enabled);
}
