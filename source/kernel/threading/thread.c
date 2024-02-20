#include "thread.h"
#include "../interrupts/irq.h"
#include "../lib/id_generator.h"
#include "../scheduler/scheduler.h"
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

    // If thread has not been detached, then it is at least referenced by
    // parent(current) thread and waiting on ref count, so we can just add it as
    // a new process thread group, and it will safely exit from process
    if (child->parent != get_current_thread()
        || !process_add_thread_group(child->proc, (struct thread*) child)) {
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
    WAIT_FOR_IRQ(&refc->empty_cvar, &current->lock, interrupts_enabled,
                 refc->count == 0);

    current->state = DEAD;
    schedule_thread_exit();
    spin_unlock(&current->lock);

    thread_cleaner_mark(current);
    local_irq_restore(interrupts_enabled);

    schedule();
}

void thread_group_signal(thread* thread_group, signal sig) {
    bool interrupts_enabled = spin_lock_irq_save(&thread_group->lock);
    thread_signal(thread_group, sig);
    ARRAY_LIST_FOR_EACH(&thread_group->children, thread * iter) {
        spin_unlock_irq_restore(&thread_group->lock, interrupts_enabled);
        thread_group_signal(iter, sig);

        interrupts_enabled = spin_lock_irq_save(&thread_group->lock);
    }
}

void thread_destroy(thread* thrd) {
    process_exit_thread(thrd->proc, (struct thread*) thrd);

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
    if (!thrd->exiting && signal_allowed(thrd->signal_info.signals_mask, sig)) {
        signal_raise(&thrd->signal_info.pending_signals, sig);
        signal_set = true;
    }
    spin_unlock_irq_restore(&thrd->lock, interrupts_enabled);

    return signal_set;
}

bool thread_set_sigaction(thread* thrd, signal sig, sigaction action) {
    bool interrupts_enabled = spin_lock_irq_save(&thrd->lock);
    bool action_set = signal_set_action(&thrd->signal_info, sig, action);
    spin_unlock_irq_restore(&thrd->lock, interrupts_enabled);

    return action_set;
}