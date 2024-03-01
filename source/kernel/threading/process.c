#include "process.h"
#include "../arch/x86_64/cpu/cpu_context.h"
#include "../error/errno.h"
#include "../memory/virtual/vmm.h"
#include "../scheduler/scheduler.h"
#include "threading.h"
#include "uthread.h"

bool process_init(process* proc, bool kernel_process) {
    if (!threading_allocate_pid(&proc->id))
        goto failed_to_allocate_pid;

    if (!array_list_init(&proc->threads, 8))
        goto failed_to_init_thread_list;

    if (!array_list_init(&proc->children, 8))
        goto failed_to_init_children_list;

    proc->vm = kernel_process ? vmm_kernel_vm_space()
                              : vm_space_fork(vmm_current_vm_space());

    if (!proc->vm)
        goto failed_to_fork_vm_space;

    if (!id_generator_init(&proc->tgid_generator))
        goto failed_to_init_tgid_generator;

    proc->cleaner_node = (linked_list_node) LINKED_LIST_NODE_OF(proc);

    proc->kernel_process = kernel_process;
    proc->lock = SPIN_LOCK_STATIC_INITIALIZER;
    proc->refc = (ref_count) REF_COUNT_STATIC_INITIALIZER;
    proc->exiting = false;
    proc->finished = false;
    memset(&proc->siginfo, 0, sizeof(process_siginfo));

    // TODO: add to parent

    return true;

failed_to_init_tgid_generator:
    vm_space_destroy(proc->vm);

failed_to_fork_vm_space:
    array_list_deinit(&proc->children);

failed_to_init_children_list:
    array_list_deinit(&proc->threads);

failed_to_init_thread_list:
    threading_free_pid(proc->id);

failed_to_allocate_pid:
    return false;
}

process* process_create() {
    process* proc = kmalloc(sizeof(process));
    if (!proc)
        return NULL;

    if (!process_init(proc, false)) {
        kfree(proc);
        return NULL;
    }

    return proc;
}

void process_destroy(process* proc) {
    if (proc->kernel_process)
        panic("Trying to destroy kernel process");

    vm_space_destroy(proc->vm);

    threading_free_pid(proc->id);

    id_generator_deinit(&proc->tgid_generator);

    array_list_deinit(&proc->threads);
    array_list_deinit(&proc->children);

    kfree(proc);
}

u64 process_fork(struct cpu_context* context) {
    thread* current = get_current_thread();
    process* proc = current->proc;

    process* created = process_create();
    if (!created)
        goto failed_to_create_process;

    if (!id_generator_get_id(&created->tgid_generator, &current->tgid))
        goto failed_to_set_created_process_start_thread_tgid;

    cpu_context* current_arch_context = (cpu_context*) context;

    thread* start_thread =
        uthread_create_orphan(created, "main", current->user_stack,
                              (uthread_func*) current_arch_context->rip);
    if (!start_thread)
        goto failed_to_create_start_process_thread;

    bool interrupts_enabled = spin_lock_irq_save(&proc->lock);
    memcpy(&created->siginfo.dispositions, &proc->siginfo.dispositions,
           sizeof(signal_disposition) * (SIGNALS_COUNT + 1));
    array_list_add_last(&proc->children, created);
    spin_unlock_irq_restore(&proc->lock, interrupts_enabled);

    cpu_context* forked_arch_context = (cpu_context*) start_thread->context;
    *forked_arch_context = *current_arch_context;
    forked_arch_context->rax = 0;

    thread_start(start_thread);

    return created->id;

failed_to_create_start_process_thread:
failed_to_set_created_process_start_thread_tgid:

failed_to_create_process:
    return -ENOMEM;
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

    // Unmap user stack for user threads
    // should be performed before releasing tgid to avoid race condition,
    // if other thread obtains released tgid and maps that area before we
    // unmapped it
    if (!current->kernel_thread) {
        rw_spin_lock_write(&proc->vm->lock);
        vm_space_unmap_pages(proc->vm, (u64) current->user_stack,
                             USER_STACK_PAGE_COUNT);
        rw_spin_unlock_write(&proc->vm->lock);
    }

    id_generator_free_id(&proc->tgid_generator, current->tgid);

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