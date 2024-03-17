#include "process.h"
#include "../arch/x86_64/cpu/cpu_context.h"
#include "../error/errno.h"
#include "../interrupts/irq.h"
#include "../memory/virtual/vmm.h"
#include "../synchronization/wait.h"
#include "scheduler.h"
#include "thread_cleaner.h"
#include "threading.h"
#include "uthread.h"

#define MAX_PROC 4096

process* init_process;
static process* process_list[MAX_PROC];

/*
 *  lock order:
 *  process_table_lock ->
 *  init_process lock ->
 *  parent lock ->
 *  child lock
 */
static lock process_table_lock = SPIN_LOCK_STATIC_INITIALIZER;

static bool add_process_to_table(process* proc);
static bool remove_process_from_table(process* proc);

static bool process_add_child(process* child);
static void process_remove_child(process* child);
// Assumes that current process lock is held and interrupts are disabled
static void process_remove_child_unsafe(process* child);

bool process_init(process* parent, process* proc, bool kernel_process) {
    if (!threading_allocate_pid(&proc->id))
        goto failed_to_allocate_pid;

    if (!array_list_init(&proc->threads, 8))
        goto failed_to_init_thread_list;

    linked_list_init(&proc->children);

    proc->vm = kernel_process ? vmm_kernel_vm_space()
                              : vm_space_fork(vmm_current_vm_space());

    if (!proc->vm)
        goto failed_to_fork_vm_space;

    if (!id_generator_init(&proc->tgid_generator))
        goto failed_to_init_tgid_generator;

    proc->process_node = (linked_list_node) LINKED_LIST_NODE_OF(proc);

    proc->kernel_process = kernel_process;
    proc->lock = SPIN_LOCK_STATIC_INITIALIZER;
    proc->refc = (ref_count) REF_COUNT_STATIC_INITIALIZER;
    proc->exiting = false;
    proc->finished = false;
    memset(&proc->siginfo, 0, sizeof(process_siginfo));

    if (parent && !process_add_child(proc))
        goto failed_to_add_process_to_parent;

    if (!add_process_to_table(proc))
        goto failed_to_add_process_to_table;

    return true;

failed_to_add_process_to_table:
    if (parent) {
        process_remove_child(proc);
    }

failed_to_add_process_to_parent:
    id_generator_deinit(&proc->tgid_generator);

failed_to_init_tgid_generator:
    vm_space_destroy(proc->vm);

failed_to_fork_vm_space:
    array_list_deinit(&proc->threads);

failed_to_init_thread_list:
    threading_free_pid(proc->id);

failed_to_allocate_pid:
    return false;
}

static process* create_user_process() {
    process* proc = kmalloc(sizeof(process));
    if (!proc)
        return NULL;

    thread* current = get_current_thread();
    process* parent = current ? current->proc : NULL;
    if (!process_init(parent, proc, false)) {
        kfree(proc);
        return NULL;
    }

    return proc;
}

process* create_user_init_process() {
    init_process = create_user_process();
    if (!init_process)
        panic("Can't create user init process");

    return init_process;
}

void process_destroy(process* proc) {
    remove_process_from_table(proc);

    vm_space_destroy(proc->vm);

    threading_free_pid(proc->id);

    id_generator_deinit(&proc->tgid_generator);

    array_list_deinit(&proc->threads);

    kfree(proc);
}

bool process_add_thread(process* proc, struct thread* thrd) {
    bool interrupts_enabled = spin_lock_irq_save(&proc->lock);
    bool added = !proc->exiting && array_list_add_last(&proc->threads, thrd);
    spin_unlock_irq_restore(&proc->lock, interrupts_enabled);

    return added;
}

u64 process_get_id(process* proc) {
    bool interrupts_enabled = spin_lock_irq_save(&proc->lock);
    u64 pid = proc->id;
    spin_unlock_irq_restore(&proc->lock, interrupts_enabled);

    return pid;
}

u64 process_fork(struct cpu_context* context) {
    thread* current = get_current_thread();
    process* proc = current->proc;

    process* created = create_user_process();
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
    memcpy(&created->siginfo, &proc->siginfo, sizeof(process_siginfo));
    created->siginfo.pending_signals = PENDING_SIGNALS_CLEAR;
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

static bool process_is_finished(process* proc) {
    bool interrupts_enabled = spin_lock_irq_save(&proc->lock);
    bool finished = proc->finished;
    spin_unlock_irq_restore(&proc->lock, interrupts_enabled);

    return finished;
}

u64 process_wait(u64 pid, u64* exit_code) {
    process* proc = get_current_thread()->proc;
    linked_list_node* exited_node = NULL;
    bool interrupted;

    bool interrupts_enabled = spin_lock_irq_save(&proc->lock);
    if (!pid) {
        interrupted = WAIT_FOR_IRQ_INTERRUPTABLE(
            &proc->lock, interrupts_enabled,
            proc->children.size == 0
                || (exited_node = LINKED_LIST_FIND(
                        &proc->children, child_node,
                        process_is_finished(child_node->value))));
    } else {
        interrupted = WAIT_FOR_IRQ_INTERRUPTABLE(
            &proc->lock, interrupts_enabled,
            proc->children.size == 0
                || !(exited_node = LINKED_LIST_FIND(
                         &proc->children, child_node,
                         ((process*) child_node->value)->id == pid))
                || process_is_finished(exited_node->value));
    }

    if (!interrupted && proc->children.size == 0) {
        spin_unlock_irq_restore(&proc->lock, interrupts_enabled);
        return -ECHILD;
    } else if (!interrupted && !exited_node) {
        spin_unlock_irq_restore(&proc->lock, interrupts_enabled);
        return -EINVAL;
    } else if (interrupted) {
        spin_unlock_irq_restore(&proc->lock, interrupts_enabled);
        return -EINTR;
    }

    process* exited = exited_node->value;
    spin_lock(&exited->lock);
    u64 exit_pid = exited->id;
    *exit_code = exited->exit_code;
    spin_unlock(&exited->lock);

    process_remove_child_unsafe(exited);
    spin_unlock_irq_restore(&proc->lock, interrupts_enabled);

    return exit_pid;
}

static void process_transfer_child_to_init(process* child) {
    process* proc = get_current_thread()->proc;
    if (proc == init_process)
        panic("Trying to transfer child from init to init");
    if (proc->kernel_process)
        panic("Trying to transfer child from kernel process");

    bool interrupts_enabled = spin_lock_irq_save(&init_process->lock);
    spin_lock(&proc->lock);
    spin_lock(&child->lock);

    if (child->parent != proc)
        panic("Trying to transfer process which is not child");

    linked_list_remove_node(&proc->children, &child->process_node);
    ref_release(&proc->refc);

    child->parent = init_process;
    linked_list_add_last_node(&init_process->children, &child->process_node);
    ref_acquire(&init_process->refc);

    spin_unlock(&child->lock);
    spin_unlock(&proc->lock);
    spin_unlock_irq_restore(&init_process->lock, interrupts_enabled);

    process_signal(init_process, SIGCHLD);
}

static void process_transfer_children_to_init() {
    process* current = get_current_thread()->proc;

    bool interrupts_enabled = spin_lock_irq_save(&current->lock);
    while (current->children.size != 0) {
        process* child = linked_list_first(&current->children);
        spin_unlock_irq_restore(&current->lock, interrupts_enabled);

        // It is safe to break atomicity in this case, since this function will
        // be invoked in last alive thread
        process_transfer_child_to_init(child);

        interrupts_enabled = spin_lock_irq_save(&current->lock);
    }
    spin_unlock_irq_restore(&current->lock, interrupts_enabled);
}

static void process_finalize() {
    process* proc = get_current_thread()->proc;
    if (proc == init_process)
        panic("Trying to destroy init process");
    if (proc->kernel_process)
        panic("Trying to destroy kernel process");

    bool interrupts_enabled = spin_lock_irq_save(&process_table_lock);
    spin_lock(&proc->lock);
    process* parent = proc->parent;
    spin_unlock(&proc->lock);

    // Since we have locked process table, process pointer we read is still
    // valid. (Because at the time we read it from proc->parent it was still
    // alive, and for it to be invalid it has to take process table lock).
    //
    // True parent may have been changed (since parent may have passed
    // current process to init), but we should not bother signaling exact
    // parent, since if current thread has been moved to init as child, it
    // will be signaled anyway.
    if (parent) {
        process_signal(parent, SIGCHLD);
    }
    spin_unlock_irq_restore(&process_table_lock, interrupts_enabled);

    process_transfer_children_to_init();

    interrupts_enabled = spin_lock_irq_save(&proc->lock);
    ref_count* refc = &proc->refc;
    CON_VAR_WAIT_FOR_IRQ(&refc->empty_cvar, &proc->lock, interrupts_enabled,
                         refc->count == 0);

    vmm_switch_to_kernel_vm_space();
    process_destroy(proc);
}

_Noreturn void process_exit_thread() {
    thread* current = get_current_thread();
    process* proc = current->proc;

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

    // last leaving user thread awaits for process refcount to reach zero
    // and cleans up process
    if (!proc->kernel_process && proc->threads.size == 0) {
        proc->finished = true;
        con_var_broadcast(&proc->finish_cvar);

        // It's ok to break atomicity, since last alive thread is running
        // anyway, and no other threads will be added at this point
        spin_unlock_irq_restore(&proc->lock, interrupts_enabled);

        process_finalize();
    }

    // Thread exiting should be performed on kernel vm, since after process lock
    // unlock, its vm may be destroyed while current thread is still exiting
    local_irq_disable();
    vmm_switch_to_kernel_vm_space();

    // At this point thread won't use any process resources, and will finish its
    // execution in a couple of process cycles, so it is safe to release process
    // lock
    spin_unlock(&proc->lock);

    spin_lock(&current->lock);
    current->state = DEAD;
    thread_cleaner_mark(current);
    schedule_thread_exit();

    __builtin_unreachable();
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

bool process_set_sigaction(signal sig, sigaction action) {
    process* proc = get_current_thread()->proc;
    bool action_set = false;

    bool interrupts_enabled = spin_lock_irq_save(&proc->lock);
    if (!proc->exiting && sig != SIGKILL) {
        proc->siginfo.actions[sig] = action;
        action_set = true;
    }
    spin_unlock_irq_restore(&proc->lock, interrupts_enabled);

    return action_set;
}

sigaction process_get_sigaction(signal sig) {
    process* proc = get_current_thread()->proc;

    bool interrupts_enabled = spin_lock_irq_save(&proc->lock);
    sigaction action = proc->siginfo.actions[sig];
    spin_unlock_irq_restore(&proc->lock, interrupts_enabled);

    return action;
}

bool process_any_pending_signals() {
    process* proc = get_current_thread()->proc;

    bool interrupts_enabled = spin_lock_irq_save(&proc->lock);
    bool any_pending = proc->siginfo.pending_signals != PENDING_SIGNALS_CLEAR;
    spin_unlock_irq_restore(&proc->lock, interrupts_enabled);

    return any_pending;
}

void process_kill(process* proc) {
    if (proc->kernel_process)
        panic("Trying to kill kernel process");

    process_signal(proc, SIGKILL);
}

static bool process_add_child(process* child) {
    process* proc = get_current_thread()->proc;

    if (proc->kernel_process)
        panic("Trying to add child to kernel process");

    bool interrupts_enabled = spin_lock_irq_save(&proc->lock);
    bool added = !proc->exiting;
    if (added) {
        linked_list_add_last_node(&proc->children, &child->process_node);
        child->parent = proc;
        ref_acquire(&proc->refc);
        // No locking required since this function is called on child thread
        // creation
        ref_acquire(&child->refc);
    }
    spin_unlock_irq_restore(&proc->lock, interrupts_enabled);

    return added;
}

// Assumes that current process lock is held and interrupts are disabled
static void process_remove_child_unsafe(process* child) {
    process* proc = get_current_thread()->proc;

    spin_lock(&child->lock);

    ref_release(&proc->refc);
    child->parent = NULL;
    ref_release(&child->refc);
    spin_unlock(&child->lock);

    linked_list_remove_node(&proc->children, &child->process_node);
}

static void process_remove_child(process* child) {
    process* proc = get_current_thread()->proc;
    bool interrupts_enabled = spin_lock_irq_save(&proc->lock);
    process_remove_child_unsafe(child);
    spin_unlock_irq_restore(&proc->lock, interrupts_enabled);
}

static bool add_process_to_table(process* proc) {
    bool interrupts_enabled = spin_lock_irq_save(&process_table_lock);
    process_list[proc->id] = proc;
    spin_unlock_irq_restore(&process_table_lock, interrupts_enabled);

    return true;
}

static bool remove_process_from_table(process* proc) {
    bool interrupts_enabled = spin_lock_irq_save(&process_table_lock);
    process_list[proc->id] = NULL;
    spin_unlock_irq_restore(&process_table_lock, interrupts_enabled);

    return true;
}