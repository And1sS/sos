#include "thread.h"
#include "../lib/id_generator.h"
#include "../scheduler/scheduler.h"
#include "thread_cleaner.h"
#include "threading.h"

void thread_start(thread* thrd) { schedule_thread(thrd); }

u64 thread_join(thread* child) {
    bool interrupts_enabled = spin_lock_irq_save(&child->lock);
    WAIT_FOR_IRQ(&child->finish_cvar, &child->lock, interrupts_enabled,
                 child->finished);

    u64 exit_code = child->exit_code;
    spin_unlock_irq_restore(&child->lock, interrupts_enabled);

    thread_detach(child);

    return exit_code;
}

void thread_detach(thread* child) {
    bool interrupts_enabled = spin_lock_irq_save(&child->lock);
    child->parent = NULL;
    ref_release(&child->refc);
    spin_unlock_irq_restore(&child->lock, interrupts_enabled);

    thread* current = get_current_thread();
    interrupts_enabled = spin_lock_irq_save(&current->lock);

    array_list_remove(&current->children, child);
    ref_release(&current->refc);
    spin_unlock_irq_restore(&current->lock, interrupts_enabled);
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
            thread_join(child);
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
    thread_cleaner_mark(current);
    spin_unlock_irq_restore(&current->lock, interrupts_enabled);

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
    if (signal_allowed(thrd->signals_mask, sig)) {
        signal_raise(&thrd->pending_signals, sig);
        signal_set = true;
    }
    spin_unlock_irq_restore(&thrd->lock, interrupts_enabled);

    return signal_set;
}