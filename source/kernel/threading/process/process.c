#include "process.h"
#include "../../error/errno.h"
#include "../../memory/virtual/vmm.h"
#include "../../scheduler/scheduler.h"
#include "../thread/uthread.h"
#include "../threading.h"

static process* init_process;

#define MAX_PROC 4096
static process* process_table[MAX_PROC];
static lock* process_table_lock;

void set_init_process(process* proc) { init_process = proc; }

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
    proc->child_changed_cvar = (con_var) CON_VAR_STATIC_INITIALIZER;
    proc->refc = (ref_count) REF_COUNT_STATIC_INITIALIZER;
    proc->exiting = false;
    proc->finished = false;
    proc->finish_cvar = (con_var) CON_VAR_STATIC_INITIALIZER;
    memset(&proc->siginfo, 0, sizeof(process_siginfo));

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

void process_destroy(process* proc) {
    if (proc->kernel_process)
        panic("Trying to destroy kernel process");

    if (proc->id == 1)
        panic("Trying to destroy init process");

    vm_space_destroy(proc->vm);

    threading_free_pid(proc->id);

    id_generator_deinit(&proc->tgid_generator);

    array_list_deinit(&proc->threads);
    array_list_deinit(&proc->children);

    kfree(proc);
}

bool process_add_child(process* child) {
    process* proc = get_current_thread()->proc;

    if (proc->kernel_process)
        panic("Trying to add child to kernel process");

    bool added = false;

    bool interrupts_enabled = spin_lock_irq_save(&proc->lock);
    if (!proc->exiting && array_list_add_last(&proc->children, child)) {
        child->parent = proc;
        ref_acquire(&proc->refc);
        // No locking required since this function is called on child thread
        // creation
        ref_acquire(&child->refc);
        added = true;
    }
    spin_unlock_irq_restore(&proc->lock, interrupts_enabled);

    return added;
}

bool process_remove_child(process* child) {
    process* proc = get_current_thread()->proc;
    if (proc->kernel_process)
        panic("Trying to remove child from kernel process");

    bool removed = false;
    bool interrupts_enabled = spin_lock_irq_save(&proc->lock);
    if (child->parent != proc) {
        spin_unlock_irq_restore(&proc->lock, interrupts_enabled);
        return false;
    }

    if (array_list_remove(&proc->children, child)) {
        ref_release(&proc->refc);
        removed = true;
    }
    spin_unlock_irq_restore(&proc->lock, interrupts_enabled);

    if (removed) {
        interrupts_enabled = spin_lock_irq_save(&child->lock);
        child->parent = NULL;
        ref_release(&child->refc);
        spin_unlock_irq_restore(&child->lock, interrupts_enabled);
    }

    return removed;
}

static void process_transfer_child_to_init(process* child) {
    process* proc = get_current_thread()->proc;
    if (proc->kernel_process)
        panic("Trying to transfer child from kernel process");

    bool interrupts_enabled = spin_lock_irq_save(&proc->lock);
    if (child->parent != proc)
        panic("Trying to transfer process which is not child");

    array_list_remove(&proc->children, child);
    ref_release(&proc->refc);
    spin_unlock_irq_restore(&proc->lock, interrupts_enabled);

    interrupts_enabled = spin_lock_irq_save(&child->lock);
    child->parent = init_process;
    spin_unlock_irq_restore(&child->lock, interrupts_enabled);

    interrupts_enabled = spin_lock_irq_save(&init_process->lock);
    // TODO: This may fail, but it has no right to do so, so replace array list
    // with linked list and static node inside process, so that this will never
    // fail
    array_list_add_last(&init_process->children, child);
    ref_acquire(&init_process->refc);
    con_var_broadcast(&init_process->child_changed_cvar);
    spin_unlock_irq_restore(&init_process->lock, interrupts_enabled);

    process_signal(init_process, SIGCHLD);
}

static void process_transfer_children_to_init() {
    process* current = get_current_thread()->proc;

    bool interrupts_enabled = spin_lock_irq_save(&current->lock);
    while (current->children.size != 0) {
        process* child = array_list_get(&current->children, 0);
        spin_unlock_irq_restore(&current->lock, interrupts_enabled);

        process_transfer_child_to_init(child);

        interrupts_enabled = spin_lock_irq_save(&current->lock);
    }
    spin_unlock_irq_restore(&current->lock, interrupts_enabled);
}

bool process_wait(process* proc) {
    process* current = get_current_thread()->proc;

    bool interrupts_enabled = spin_lock_irq_save(&proc->lock);
    if (proc->parent != current) {
        spin_unlock_irq_restore(&proc->lock, interrupts_enabled);
        return false;
    }

    WAIT_FOR_IRQ(&proc->finish_cvar, &proc->lock, interrupts_enabled,
                 proc->finished);
    spin_unlock_irq_restore(&proc->lock, interrupts_enabled);

    return process_remove_child(proc);
}

static bool any_child_exited(array_list* children, process** finished) {
    ARRAY_LIST_FOR_EACH(children, process * iter) {
        bool interrupts_enabled = spin_lock_irq_save(&iter->lock);
        if (iter->finished) {
            *finished = iter;
            spin_unlock_irq_restore(&iter->lock, interrupts_enabled);
            return true;
        }

        spin_unlock_irq_restore(&iter->lock, interrupts_enabled);
    }

    return false;
}

u64 process_wait_any(u64* exit_code) {
    process* current = get_current_thread()->proc;

    bool interrupts_enabled = spin_lock_irq_save(&current->lock);
    //    if (current->children.size == 0) {
    //        spin_unlock_irq_restore(&current->lock, interrupts_enabled);
    //        return -ECHILD;
    //    }

    process* exited;
    u64 exit_pid;

    WAIT_FOR_IRQ(&current->child_changed_cvar, &current->lock,
                 interrupts_enabled,
                 any_child_exited(&current->children, &exited));

    spin_lock(&exited->lock);
    exit_pid = exited->id;
    *exit_code = exited->exit_code;
    spin_unlock(&exited->lock);

    spin_unlock_irq_restore(&current->lock, interrupts_enabled);

    return process_remove_child(exited) ? exit_pid : (u64) -ECHILD;
}

bool process_add_thread(process* proc, struct thread* thrd) {
    bool interrupts_enabled = spin_lock_irq_save(&proc->lock);
    if (proc->exiting)
        panic("Trying to add new thread to finishing process");

    bool added = array_list_add_last(&proc->threads, thrd);
    spin_unlock_irq_restore(&proc->lock, interrupts_enabled);

    return added;
}

void process_signal_parent(signal sig) {
    process* current = get_current_thread()->proc;
    bool exit = false;

    while (!exit) {
        bool interrupts_enabled = spin_lock_irq_save(&current->lock);
         = current->parent->
        spin_unlock_irq_restore(&current->lock, interrupts_enabled);
    }
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
        process* parent = proc->parent;
        spin_unlock_irq_restore(&proc->lock, interrupts_enabled);

        process_signal(parent, SIGCHLD);

        process_transfer_children_to_init();

        interrupts_enabled = spin_lock_irq_save(&proc->lock);
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