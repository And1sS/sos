#include "process.h"
#include "../lib/kprint.h"
#include "../memory/heap/kheap.h"
#include "../memory/virtual/vmm.h"
#include "../scheduler/scheduler.h"
#include "threading.h"

bool process_init(process* proc, bool kernel_process) {
    if (!threading_allocate_pid(&proc->id))
        goto failed_to_allocate_pid;

    if (!array_list_init(&proc->threads, 8))
        goto failed_to_init_thread_list;

    vm_space* kernel_vm = vmm_kernel_vm_space();
    proc->vm = kernel_process ? kernel_vm : vm_space_fork(kernel_vm);
    if (!proc->vm)
        goto failed_to_fork_vm_space;

    proc->cleaner_node = (linked_list_node) LINKED_LIST_NODE_OF(proc);

    proc->kernel_process = kernel_process;
    proc->lock = SPIN_LOCK_STATIC_INITIALIZER;
    proc->refc = (ref_count) REF_COUNT_STATIC_INITIALIZER;
    proc->exiting = false;
    proc->finished = false;
    memset(&proc->siginfo, 0, sizeof(process_siginfo));

    return true;

failed_to_fork_vm_space:
    array_list_deinit(&proc->threads);

failed_to_init_thread_list:
    threading_free_pid(proc->id);

failed_to_allocate_pid:
    return false;
}

void process_destroy(process* proc) {
    if (proc->kernel_process)
        panic("Trying to destroy kernel process");

    print("Destroying process!");

    vm_space_destroy(proc->vm);
    threading_free_pid(proc->id);
    kfree(proc);
}

bool process_add_thread(process* proc, struct thread* thrd) {
    bool interrupts_enabled = spin_lock_irq_save(&proc->lock);
    if (proc->exiting)
        panic("Trying to add new thread to finishing process");

    bool added = array_list_add_last(&proc->threads, thrd);
    spin_unlock_irq_restore(&proc->lock, interrupts_enabled);

    return added;
}

bool process_exit_thread() {
    thread* current = get_current_thread();
    process* proc = current->proc;

    bool interrupts_enabled = spin_lock_irq_save(&proc->lock);
    proc->exiting = true;
    ref_release(&proc->refc);

    // some thread has removed current thread already, this means that it will
    // be responsible for cleanup
    if (!array_list_remove(&proc->threads, current)) {
        spin_unlock_irq_restore(&proc->lock, interrupts_enabled);
        return false;
    }

    // last leaving thread awaits for process refcount to reach zero
    if (!proc->kernel_process && proc->threads.size == 0) {
        proc->finished = true;
        con_var_broadcast(&proc->finish_cvar);

        // TODO: send SIGCHLD to parent process

        ref_count* refc = &proc->refc;
        WAIT_FOR_IRQ(&refc->empty_cvar, &proc->lock, interrupts_enabled,
                     refc->count == 0);

        spin_unlock_irq_restore(&proc->lock, interrupts_enabled);
        return true;
    }

    spin_unlock_irq_restore(&proc->lock, interrupts_enabled);
    return false;
}

void process_exit(u64 exit_code) {
    thread* current = get_current_thread();
    process* proc = current->proc;

    bool interrupts_enabled = spin_lock_irq_save(&proc->lock);
    if (proc->exiting) {
        spin_unlock_irq_restore(&proc->lock, interrupts_enabled);
        thread_exit(exit_code);
    }

    proc->exiting = true;
    proc->exit_code = exit_code;

    // if thread pointer is still inside thread group list, then it's still
    // valid for the duration of holding process lock
    thread* iter = (thread*) array_list_remove_first(&proc->threads);
    while (iter) {
        if (iter == current) {
            iter = (thread*) array_list_remove_first(&proc->threads);
            continue;
        }

        spin_lock(&iter->lock);
        // unlock should be only done while holding thread lock, to avoid
        // race condition with thread exiting (in particular, thread pointer
        // may be invalid after call to process_exit_thread in thread_exit)
        spin_unlock(&proc->lock);

        // signal SIGKILL all unfinished threads, including current thread
        if (!iter->finished) {
            // acquire reference, so thread won't be freed until we are done
            // with it
            ref_acquire(&iter->refc);
            spin_unlock_irq_restore(&iter->lock, interrupts_enabled);

            thread_signal(iter, SIGKILL);
            thread_lock_guarded_refc_release(iter, &iter->refc);
        } else {
            spin_unlock_irq_restore(&iter->lock, interrupts_enabled);
        }
        interrupts_enabled = spin_lock_irq_save(&proc->lock);

        iter = (thread*) array_list_remove_first(&proc->threads);
    }

    // TODO: Pass child processes to init
    // Add current thread back, so that it will be last thread and will perform
    // cleanup
    array_list_add_first(&proc->threads, current);
    spin_unlock_irq_restore(&proc->lock, interrupts_enabled);
    thread_exit(exit_code);
}

bool process_signal(process* proc, signal sig) {
    if (proc->kernel_process)
        return false;

    bool interrupts_enabled = spin_lock_irq_save(&proc->lock);
    if (proc->exiting) {
        spin_unlock_irq_restore(&proc->lock, interrupts_enabled);
        return false;
    }

    ARRAY_LIST_FOR_EACH(&proc->threads, thread * iter) {
        if (thread_signal_if_allowed(iter, sig)) {
            spin_unlock_irq_restore(&proc->lock, interrupts_enabled);
            return true;
        }
    }

    signal_raise(&proc->siginfo.pending_signals, sig);
    spin_unlock_irq_restore(&proc->lock, interrupts_enabled);

    return true;
}

bool process_set_signal_disposition(signal sig,
                                    signal_disposition disposition) {

    process* proc = get_current_thread()->proc;
    bool disposition_set = false;

    bool interrupts_enabled = spin_lock_irq_save(&proc->lock);
    if (!proc->exiting && sig != SIGKILL) {
        proc->siginfo.dispositions[sig] = disposition;
        disposition_set = true;
    }
    spin_unlock_irq_restore(&proc->lock, interrupts_enabled);

    return disposition_set;
}

signal_disposition process_get_signal_disposition(signal sig) {
    process* proc = get_current_thread()->proc;

    bool interrupts_enabled = spin_lock_irq_save(&proc->lock);
    signal_disposition disposition = proc->siginfo.dispositions[sig];
    spin_unlock_irq_restore(&proc->lock, interrupts_enabled);

    return disposition;
}

void process_kill(process* proc) {
    if (proc->kernel_process)
        panic("Trying to kill kernel process");

    process_signal(proc, SIGKILL);
}