#include "thread.h"
#include "../interrupts/irq.h"
#include "../lib/id_generator.h"
#include "../lib/kprint.h"
#include "../scheduler/scheduler.h"
#include "cleaners/process_cleaner.h"
#include "cleaners/thread_cleaner.h"
#include "threading.h"

void thread_start(thread* thrd) { schedule_thread(thrd); }

bool thread_add_child(thread* child) {
    thread* current = get_current_thread();
    bool interrupts_enabled = spin_lock_irq_save(&current->lock);

    if (!array_list_add_last(&current->children, child)) {
        spin_unlock_irq_restore(&current->lock, interrupts_enabled);
        return false;
    }

    // No locking required since this function is called on child thread
    // creation
    child->parent = current;
    ref_acquire(&child->refc);
    ref_acquire(&current->refc);
    spin_unlock_irq_restore(&current->lock, interrupts_enabled);

    return true;
}

void thread_remove_child(thread* child) {
    thread* current = get_current_thread();

    bool interrupts_enabled = spin_lock_irq_save(&child->lock);
    child->parent = NULL;
    spin_unlock_irq_restore(&child->lock, interrupts_enabled);
    thread_lock_guarded_refc_release(child, &child->refc);

    interrupts_enabled = spin_lock_irq_save(&current->lock);

    array_list_remove(&current->children, child);
    spin_unlock_irq_restore(&current->lock, interrupts_enabled);

    thread_lock_guarded_refc_release(current, &current->refc);
}

bool thread_detach(thread* child) {
    bool interrupts_enabled = spin_lock_irq_save(&child->lock);
    if (child->parent != get_current_thread()) {
        spin_unlock_irq_restore(&child->lock, interrupts_enabled);
        return false;
    }

    // It is safe to break atomicity in this case, since only parent thread
    // can detach/join/remove child
    spin_unlock_irq_restore(&child->lock, interrupts_enabled);

    thread_remove_child(child);
    return true;
}

bool thread_join(thread* child, u64* exit_code) {
    bool interrupts_enabled = spin_lock_irq_save(&child->lock);
    if (child->parent != get_current_thread()) {
        spin_unlock_irq_restore(&child->lock, interrupts_enabled);
        return false;
    }

    WAIT_FOR_IRQ(&child->finish_cvar, &child->lock, interrupts_enabled,
                 child->finished);

    if (exit_code) {
        *exit_code = child->exit_code;
    }

    spin_unlock_irq_restore(&child->lock, interrupts_enabled);

    // It is safe to break atomicity in this case, since only parent thread
    // can detach/join/remove child
    thread_remove_child(child);

    return true;
}

void thread_exit(u64 exit_code) {
    thread* current = get_current_thread();
    process* proc = current->proc;

    bool interrupts_enabled = spin_lock_irq_save(&current->lock);
    current->exiting = true;
    current->exit_code = exit_code;

    while (current->children.size != 0) {
        thread* child = array_list_remove_first(&current->children);
        spin_unlock_irq_restore(&current->lock, interrupts_enabled);

        thread_join(child, NULL);

        interrupts_enabled = spin_lock_irq_save(&current->lock);
    }

    current->finished = true;
    spin_unlock_irq_restore(&current->lock, interrupts_enabled);

    con_var_broadcast_guarded(&current->finish_cvar, &current->lock);

    interrupts_enabled = spin_lock_irq_save(&current->lock);
    ref_count* refc = &current->refc;
    WAIT_FOR_IRQ(&refc->empty_cvar, &current->lock, interrupts_enabled,
                 refc->count == 0);

    spin_unlock_irq_restore(&current->lock, interrupts_enabled);

    // Should clean process information before thread exiting
    bool cleanup_process = process_exit_thread();

    spin_lock_irq_save(&current->lock);
    current->state = DEAD;
    spin_unlock(&current->lock);

    schedule_thread_exit();

    thread_cleaner_mark(current);
    if (cleanup_process)
        process_cleaner_mark(proc);

    local_irq_restore(interrupts_enabled);

    schedule();
}

void thread_destroy(thread* thrd) {
    threading_free_tid(thrd->id);
    array_list_deinit(&thrd->children);
    kfree(thrd->kernel_stack);
    // TODO: add arch cpu_context deinit function, because different
    //       architectures might want to store context not on kernel stack, and
    //       this memory won't be automatically freed with stack
    kfree(thrd);
}

void thread_yield() { schedule(); }

bool thread_signal(thread* thrd, signal sig) {
    bool signal_set = false;
    bool interrupts_enabled = spin_lock_irq_save(&thrd->lock);
    if (!thrd->exiting && !thrd->kernel_thread) {
        signal_raise(&thrd->siginfo.pending_signals, sig);
        signal_set = true;
    }
    spin_unlock_irq_restore(&thrd->lock, interrupts_enabled);

    if (signal_set)
        schedule_thread(thrd);

    return signal_set;
}

bool thread_signal_if_allowed(thread* thrd, signal sig) {
    bool signal_set = false;
    bool interrupts_enabled = spin_lock_irq_save(&thrd->lock);
    if (!thrd->exiting && signal_allowed(thrd->siginfo.signals_mask, sig)
        && !thrd->kernel_thread) {
        signal_raise(&thrd->siginfo.pending_signals, sig);
        signal_set = true;
    }
    spin_unlock_irq_restore(&thrd->lock, interrupts_enabled);

    if (signal_set)
        schedule_thread(thrd);

    return signal_set;
}

bool thread_signal_allowed(thread* thrd, signal sig) {
    bool interrupts_enabled = spin_lock_irq_save(&thrd->lock);
    bool allowed = signal_allowed(thrd->siginfo.signals_mask, sig);
    spin_unlock_irq_restore(&thrd->lock, interrupts_enabled);

    return allowed;
}

bool thread_set_sigaction(signal sig, sigaction action) {
    thread* current = get_current_thread();

    bool action_set = false;
    bool interrupts_enabled = spin_lock_irq_save(&current->lock);
    if (!current->exiting && !current->kernel_thread && sig != SIGKILL) {
        current->siginfo.signal_actions[sig] = action;
        action_set = true;
    }
    spin_unlock_irq_restore(&current->lock, interrupts_enabled);

    return action_set;
}

sigaction thread_get_sigaction(signal sig) {
    thread* current = get_current_thread();

    bool interrupts_enabled = spin_lock_irq_save(&current->lock);
    sigaction action = current->siginfo.signal_actions[sig];
    spin_unlock_irq_restore(&current->lock, interrupts_enabled);

    return action;
}

bool thread_signal_block(signal sig) {
    thread* current = get_current_thread();

    bool interrupts_enabled = spin_lock_irq_save(&current->lock);
    bool blocked = signal_block(&current->siginfo.signals_mask, sig);
    spin_unlock_irq_restore(&current->lock, interrupts_enabled);

    return blocked;
}

void thread_signal_unblock(signal sig) {
    thread* current = get_current_thread();

    bool interrupts_enabled = spin_lock_irq_save(&current->lock);
    signal_unblock(&current->siginfo.signals_mask, sig);
    spin_unlock_irq_restore(&current->lock, interrupts_enabled);
}

void thread_lock_guarded_refc_release(thread* thrd, ref_count* refc) {
    bool interrupts_enabled = spin_lock_irq_save(&thrd->lock);

    if (refc->count == 0) {
        panic("ref_count is less than 0");
    }

    u64 new_count = --refc->count;
    spin_unlock_irq_restore(&thrd->lock, interrupts_enabled);
    if (new_count == 0) {
        con_var_broadcast_guarded(&refc->empty_cvar, &thrd->lock);
    }
}