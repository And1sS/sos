#include "signal.h"
#include "../arch/common/context.h"
#include "../arch/common/signal.h"
#include "../lib/kprint.h"
#include "../lib/math.h"
#include "../threading/scheduler.h"

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
    CORE_DUMP, // Signal not supported for now
    CORE_DUMP, // Signal not supported for now
    CORE_DUMP, // Signal not supported for now
    CORE_DUMP, // Signal not supported for now
    CORE_DUMP, // Signal not supported for now
    [SIGCHLD] = IGNORE,
    [SIGCHLD + 1 ... SIGNALS_COUNT] = FALLBACK_TO_DEFAULT};

bool signal_raised(sigpending pending_signals, signal sig) {
    return (pending_signals >> sig) & 1;
}

bool signal_any_raised(sigpending pending_signals) {
    return pending_signals != PENDING_SIGNALS_CLEAR;
}

bool signal_allowed(sigmask signals_mask, signal sig) {
    return (signals_mask >> sig) & 1;
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

void signal_unblock(sigmask* signals_mask, signal sig) {
    *signals_mask |= 1 << sig;
}

static signal signal_to_handle(sigpending* pending_signals, sigmask mask) {
    if (!signal_any_raised(*pending_signals))
        return NOSIG;

    signal to_handle = lsb_u64(*pending_signals) & mask;
    if (to_handle != NOSIG)
        signal_clear(pending_signals, to_handle);

    return to_handle;
}

static signal get_and_clear_signal_to_handle() {
    thread* current = get_current_thread();
    thread_siginfo* thread_siginfo = &current->siginfo;
    signal to_handle;

    // Check thread signals first
    bool interrupts_enabled = spin_lock_irq_save(&current->siginfo_lock);
    sigmask mask = thread_siginfo->signals_mask;
    to_handle = signal_to_handle(&thread_siginfo->pending_signals, mask);
    spin_unlock_irq_restore(&current->siginfo_lock, interrupts_enabled);

    if (to_handle != NOSIG)
        return to_handle;

    // If thread does not have signals to handle, then check process signals
    process* proc = current->proc;
    process_siginfo* process_siginfo = &proc->siginfo;

    interrupts_enabled = spin_lock_irq_save(&proc->siginfo_lock);
    to_handle = signal_to_handle(&process_siginfo->pending_signals, mask);
    spin_unlock_irq_restore(&proc->siginfo_lock, interrupts_enabled);

    return to_handle;
}

void handle_signal(signal sig, struct cpu_context* context) {
    sigaction action = process_get_sigaction(sig);
    signal_handler* handler = action.handler;

    if (handler) {
        arch_enter_signal_handler(context, handler);
        return;
    }

    signal_disposition disposition = action.disposition != FALLBACK_TO_DEFAULT
                                         ? action.disposition
                                         : default_dispositions[sig];

    switch (disposition) {
    case FALLBACK_TO_DEFAULT:
        panic("Signal handling error, resolved disposition is fallback");

    case CORE_DUMP: {
        println("Core dumped: ");
        arch_print_cpu_context(context);
        __attribute__((fallthrough));
    }

    case TERMINATE:
        process_exit(128 + sig);

    case STOP:
    case IGNORE:
    case CONTINUE:
        break;
    }
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