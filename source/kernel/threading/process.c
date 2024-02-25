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
    bool destroy = false;

    bool interrupts_enabled = spin_lock_irq_save(&proc->lock);
    ref_release(&proc->refc);
    array_list_remove(&proc->threads, current);

    // last leaving thread awaits for process refcount to reach zero
    if (!proc->kernel_process && proc->threads.size == 0) {
        proc->finished = true;
        con_var_broadcast(&proc->finish_cvar);

        // TODO: send SIGCHLD to parent process

        ref_count* refc = &proc->refc;
        WAIT_FOR_IRQ(&refc->empty_cvar, &proc->lock, interrupts_enabled,
                     refc->count == 0);

        destroy = true;
    }
    spin_unlock_irq_restore(&proc->lock, interrupts_enabled);

    return destroy;
}

void process_exit(u64 exit_code) {
    thread* current = get_current_thread();
    process* proc = current->proc;

    bool interrupts_enabled = spin_lock_irq_save(&proc->lock);
    if (!proc->exiting) {
        proc->exiting = true;
        proc->exit_code = exit_code;

        ARRAY_LIST_FOR_EACH(&proc->threads, thread * iter) {
            thread_signal(iter, SIGKILL);
        }
    }
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