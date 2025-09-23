#include "process.h"
#include "../arch/common/context.h"
#include "../error/errno.h"
#include "../lib/container/hash_table/hash_table.h"
#include "../memory/virtual/vmm.h"
#include "../synchronization/wait.h"
#include "scheduler.h"
#include "thread_cleaner.h"
#include "uthread.h"

process kernel_process;
process init_process;

static hash_table process_table;
static lock process_table_lock = SPIN_LOCK_STATIC_INITIALIZER;

static id_generator pid_gen;

static bool process_init(process* proc, bool is_kernel_process);
static bool process_add_child(process* child);
// Assumes that current process lock is held and interrupts are disabled
static void process_remove_child_unsafe(process* child);

void processing_init() {
    if (!id_generator_init(&pid_gen))
        panic("Can't init pid generator");

    if (!hash_table_init(&process_table))
        panic("Can't init process table");

    if (!process_init(&kernel_process, true))
        panic("Can't init kernel process");

    if (!process_init(&init_process, false))
        panic("Can't init user init process");

    hash_table_put(&process_table, kernel_process.id, &kernel_process, NULL);
    hash_table_put(&process_table, init_process.id, &init_process, NULL);
}

static bool process_init(process* proc, bool is_kernel_process) {
    if (!id_generator_get_id(&pid_gen, &proc->id))
        goto failed_to_allocate_pid;

    if (!array_list_init(&proc->threads, 8))
        goto failed_to_init_thread_list;

    linked_list_init(&proc->children);

    proc->vm = is_kernel_process ? vmm_kernel_vm_space()
                                 : vm_space_fork(vmm_current_vm_space());

    if (!proc->vm)
        goto failed_to_fork_vm_space;

    if (!id_generator_init(&proc->tgid_generator))
        goto failed_to_init_tgid_generator;

    proc->process_node = (linked_list_node) LINKED_LIST_NODE_OF(proc);

    proc->kernel_process = is_kernel_process;
    proc->lock = SPIN_LOCK_STATIC_INITIALIZER;
    proc->refc = (ref_count) REF_COUNT_STATIC_INITIALIZER;
    proc->exiting = false;
    proc->finished = false;
    proc->finish_cvar = (con_var) CON_VAR_STATIC_INITIALIZER;
    memset(&proc->siginfo, 0, sizeof(process_siginfo));
    proc->siginfo_lock = SPIN_LOCK_STATIC_INITIALIZER;

    return true;

failed_to_init_tgid_generator:
    vm_space_destroy(proc->vm);

failed_to_fork_vm_space:
    array_list_deinit(&proc->threads);

failed_to_init_thread_list:
    id_generator_free_id(&pid_gen, proc->id);

failed_to_allocate_pid:
    return false;
}

static process* create_user_process() {
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
    vm_space_destroy(proc->vm);
    id_generator_free_id(&pid_gen, proc->id);
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

u64 process_fork(struct cpu_context* context) {
    thread* current = get_current_thread();
    process* proc = current->proc;

    process* created = create_user_process();
    if (!created)
        goto failed_to_create_process;

    thread* start_thread = uthread_create_orphan(
        created, "main", current->user_stack,
        (uthread_func*) arch_get_instruction_pointer(context));
    if (!start_thread)
        goto failed_to_create_start_process_thread;

    bool interrupts_enabled = spin_lock_irq_save(&proc->siginfo_lock);
    memcpy(&created->siginfo, &proc->siginfo, sizeof(process_siginfo));
    created->siginfo.pending_signals = PENDING_SIGNALS_CLEAR;
    spin_unlock_irq_restore(&proc->siginfo_lock, interrupts_enabled);

    arch_clone_cpu_context(context, start_thread->context);
    arch_set_syscall_return_value(start_thread->context, 0);

    interrupts_enabled = spin_lock_irq_save(&process_table_lock);
    bool added_to_table = hash_table_put(&process_table, proc->id, proc, NULL);
    bool should_start = added_to_table && process_add_child(created);
    if (!should_start && added_to_table)
        hash_table_remove(&process_table, proc->id);
    spin_unlock_irq_restore(&process_table_lock, interrupts_enabled);

    if (!should_start)
        goto failed_to_publish;

    thread_start(start_thread);

    return created->id;

failed_to_publish:
    thread_destroy(start_thread);

failed_to_create_start_process_thread:
    process_destroy(created);

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
    if (proc == &init_process)
        panic("Trying to transfer child from init to init");
    if (proc->kernel_process)
        panic("Trying to transfer child from kernel process");

    bool interrupts_enabled = spin_lock_irq_save(&init_process.lock);
    spin_lock(&proc->lock);
    spin_lock(&child->lock);

    if (child->parent != proc)
        panic("Trying to transfer process which is not child");

    linked_list_remove_node(&proc->children, &child->process_node);
    ref_release(&proc->refc);

    child->parent = &init_process;
    linked_list_add_last_node(&init_process.children, &child->process_node);
    ref_acquire(&init_process.refc);

    spin_unlock(&child->lock);
    spin_unlock(&proc->lock);
    spin_unlock_irq_restore(&init_process.lock, interrupts_enabled);

    process_signal(&init_process, SIGCHLD);
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
    thread* current = get_current_thread();
    process* proc = current->proc;
    if (proc == &init_process)
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
    if (parent)
        process_signal(parent, SIGCHLD);
    spin_unlock_irq_restore(&process_table_lock, interrupts_enabled);

    process_transfer_children_to_init();

    interrupts_enabled = spin_lock_irq_save(&proc->lock);
    ref_count* refc = &proc->refc;

    CON_VAR_WAIT_FOR_IRQ(&refc->empty_cvar, &proc->lock, interrupts_enabled,
                         refc->count == 0);

    spin_unlock(&proc->lock); // Interrupts left disabled on purpose

    spin_lock(&process_table_lock);
    hash_table_remove(&process_table, proc->id);
    spin_unlock(&process_table_lock);

    vmm_switch_to_kernel_vm_space();
    process_destroy(proc);
}

_Noreturn void process_exit_thread() {
    thread* current = get_current_thread();
    process* proc = current->proc;

    bool interrupts_enabled = spin_lock_irq_save(&proc->lock);
    ref_release(&proc->refc);

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
    if (!proc->kernel_process && proc->threads.size == 1) {
        proc->finished = true;
        con_var_broadcast(&proc->finish_cvar);

        // It's ok to break atomicity, since last alive thread is running
        // anyway, and no other threads will be added at this point
        spin_unlock_irq_restore(&proc->lock, interrupts_enabled);

        process_finalize();
    } else {
        array_list_remove(&proc->threads, current);

        // Thread exiting should be performed on kernel vm, since after process
        // lock unlock, its vm may be destroyed while current thread is still
        // exiting
        if (!proc->kernel_process)
            vmm_switch_to_kernel_vm_space();

        // At this point thread won't use any process resources, and will finish
        // its execution in a couple of process cycles, so it is safe to release
        // process lock
        spin_unlock(&proc->lock); // Interrupts left disabled on purpose
    }

    spin_lock(&current->lock);
    current->state = DEAD;
    thread_cleaner_mark(current);
    schedule_thread_exit();
}

_Noreturn void process_exit(u64 exit_code) {
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
        if (thread_signal(iter, sig)) {
            spin_unlock_irq_restore(&proc->lock, interrupts_enabled);
            return true;
        }
    }

    spin_lock(&proc->siginfo_lock);
    signal_raise(&proc->siginfo.pending_signals, sig);
    spin_unlock(&proc->siginfo_lock);

    spin_unlock_irq_restore(&proc->lock, interrupts_enabled);

    return true;
}

bool process_set_sigaction(signal sig, sigaction action) {
    process* proc = get_current_thread()->proc;

    bool interrupts_enabled = spin_lock_irq_save(&proc->siginfo_lock);
    bool action_set = !proc->exiting && sig != SIGKILL;
    if (action_set)
        proc->siginfo.actions[sig] = action;
    spin_unlock_irq_restore(&proc->siginfo_lock, interrupts_enabled);

    return action_set;
}

sigaction process_get_sigaction(signal sig) {
    process* proc = get_current_thread()->proc;

    bool interrupts_enabled = spin_lock_irq_save(&proc->siginfo_lock);
    sigaction action = proc->siginfo.actions[sig];
    spin_unlock_irq_restore(&proc->siginfo_lock, interrupts_enabled);

    return action;
}

bool process_any_pending_signals() {
    process* proc = get_current_thread()->proc;

    bool interrupts_enabled = spin_lock_irq_save(&proc->siginfo_lock);
    bool any_pending = proc->siginfo.pending_signals != PENDING_SIGNALS_CLEAR;
    spin_unlock_irq_restore(&proc->siginfo_lock, interrupts_enabled);

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