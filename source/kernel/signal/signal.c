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

signal signal_to_handle(sigpending pending_signals, sigmask signals_mask) {
    if (!signal_any_raised(pending_signals))
        return NO_SIG;

    return lsb_u64(pending_signals) & signals_mask;
}

void signal_raise(sigpending* pending_signals, signal sig) {
    *pending_signals |= 1 << sig;
}

void signal_clear(sigpending* pending_signals, signal sig) {
    *pending_signals &= ~(1 << sig);
}

bool signal_block(sigmask* signals_mask, signal sig) {
    if (sig == SIGKILL)
        return false;

    *signals_mask &= ~(1 << sig);
    return true;
}

void signal_unblock(sigmask* signals_mask, signal sig) {
    *signals_mask |= 1 << sig;
}

static void block_and_clear_all_signals(thread* thrd) {
    thrd->siginfo.pending_signals = PENDING_SIGNALS_CLEAR;
    thrd->siginfo.signals_mask = ALL_SIGNALS_BLOCKED;
}

static signal get_and_clear_signal_to_handle() {
    thread* current = get_current_thread();
    thread_siginfo* thread_siginfo = &current->siginfo;
    signal to_handle;

    bool interrupts_enabled = spin_lock_irq_save(&current->lock);
    sigmask mask = thread_siginfo->signals_mask;
    to_handle = signal_to_handle(thread_siginfo->pending_signals, mask);

    if (to_handle != NO_SIG)
        signal_clear(&thread_siginfo->pending_signals, to_handle);
    spin_unlock_irq_restore(&current->lock, interrupts_enabled);
    if (to_handle != NO_SIG)
        return to_handle;

    process* proc = current->proc;
    process_siginfo* process_siginfo = &proc->siginfo;

    interrupts_enabled = spin_lock_irq_save(&proc->lock);
    to_handle = signal_to_handle(process_siginfo->pending_signals, mask);

    if (to_handle != NO_SIG)
        signal_clear(&process_siginfo->pending_signals, to_handle);
    spin_unlock_irq_restore(&proc->lock, interrupts_enabled);

    return to_handle;
}

static signal_disposition resolve_signal_disposition(signal sig) {
    signal_disposition disposition = process_get_signal_disposition(sig);

    return disposition != FALLBACK_TO_DEFAULT ? disposition
                                              : default_dispositions[sig];
}

void handle_pending_signals(struct cpu_context* context) {
    thread* current = get_current_thread();
    if (!current || !arch_is_userspace_context(context)) {
        return;
    }

    signal sig = get_and_clear_signal_to_handle();
    if (sig)
        handle_signal(sig, context);
}

void handle_signal(signal sig, struct cpu_context* context) {
    sigaction action = thread_get_sigaction(sig);
    signal_handler* handler = action.handler;

    if (handler) {
        arch_enter_signal_handler(context, handler);
    } else {
        signal_disposition disposition = resolve_signal_disposition(sig);
        switch (disposition) {
        case FALLBACK_TO_DEFAULT: {
            panic("Signal handling error, resolved disposition is fallback");
            __builtin_unreachable();
        }

        case CORE_DUMP: {
            println("Core dumped: ");
            arch_print_cpu_context(context);
            __attribute__((fallthrough));
        }

        case TERMINATE: {
            thread* current = get_current_thread();
            bool interrupts_enabled = spin_lock_irq_save(&current->lock);
            block_and_clear_all_signals(current);
            spin_unlock_irq_restore(&current->lock, interrupts_enabled);

            process_exit(-1);
            return;
        }

        case STOP:
        case IGNORE:
        case CONTINUE:
            break;
        }
    }
}
