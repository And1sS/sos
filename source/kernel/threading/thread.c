#include "thread.h"
#include "../interrupts/irq.h"
#include "../lib/id_generator.h"
#include "scheduler.h"
#include "thread_cleaner.h"
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
    ref_release(&child->refc);
    spin_unlock_irq_restore(&child->lock, interrupts_enabled);

    interrupts_enabled = spin_lock_irq_save(&current->lock);
    array_list_remove(&current->children, child);
    ref_release(&current->refc);
    spin_unlock_irq_restore(&current->lock, interrupts_enabled);
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

    CON_VAR_WAIT_FOR_IRQ(&child->finish_cvar, &child->lock, interrupts_enabled,
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

_Noreturn void thread_exit(u64 exit_code) {
    thread* current = get_current_thread();
    bool interrupts_enabled = spin_lock_irq_save(&current->lock);
    current->exiting = true;
    current->exit_code = exit_code;

    while (current->children.size != 0) {
        thread* child = array_list_remove_first(&current->children);

        if (child) {
            spin_unlock_irq_restore(&current->lock, interrupts_enabled);
            thread_join(child, NULL);
            interrupts_enabled = spin_lock_irq_save(&current->lock);
        }
    }

    current->finished = true;
    con_var_broadcast(&current->finish_cvar);

    ref_count* refc = &current->refc;
    CON_VAR_WAIT_FOR_IRQ(&refc->empty_cvar, &current->lock, interrupts_enabled,
                         refc->count == 0);

    spin_unlock_irq_restore(&current->lock, interrupts_enabled);
    process_exit_thread();
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

bool thread_any_pending_signals() {
    thread* current = get_current_thread();
    bool interrupts_enabled = spin_lock_irq_save(&current->lock);
    bool any_raised = signal_any_raised(current->siginfo.pending_signals
                                        & current->siginfo.signals_mask);
    spin_unlock_irq_restore(&current->lock, interrupts_enabled);
    if (any_raised)
        return true;

    return process_any_pending_signals();
}

bool thread_signal(thread* thrd, signal sig) {
    bool signal_set = false;
    bool interrupts_enabled = spin_lock_irq_save(&thrd->lock);
    if (!thrd->exiting && !thrd->kernel_thread) {
        signal_raise(&thrd->siginfo.pending_signals, sig);
        signal_set = true;
    }
    spin_unlock(&thrd->lock);

    if (signal_set)
        schedule_thread(thrd);

    local_irq_restore(interrupts_enabled);

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
    spin_unlock(&thrd->lock);

    if (signal_set)
        schedule_thread(thrd);

    local_irq_restore(interrupts_enabled);

    return signal_set;
}

void thread_unlock(thread* thrd) { spin_unlock(&thrd->lock); }