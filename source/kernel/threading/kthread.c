#include "kthread.h"
#include "../arch/common/thread.h"
#include "process.h"
#include "scheduler.h"
#include "threading.h"

// defined in process.c
extern process kernel_process;

bool kthread_init(kthread* thrd, string name, kthread_func* func) {
    memset(thrd, 0, sizeof(thread));
    if (!threading_allocate_tid(&thrd->id))
        goto failed_to_allocate_tid;

    if (!id_generator_get_id(&kernel_process.tgid_generator, &thrd->tgid))
        goto failed_to_allocate_tgid;

    void* kernel_stack = kmalloc_aligned(THREAD_KERNEL_STACK_SIZE, PAGE_SIZE);
    if (!kernel_stack)
        goto failed_to_allocate_kernel_stack;

    memset(kernel_stack, 0, THREAD_KERNEL_STACK_SIZE);

    // TODO: copy name string to kernel heap
    thrd->name = name;

    thrd->exiting = false;
    thrd->finished = false;
    thrd->exit_code = 0;

    // kernel threads never receive signals
    memset(&thrd->siginfo, 0, sizeof(thread_siginfo));
    thrd->siginfo_lock = SPIN_LOCK_STATIC_INITIALIZER;
    thrd->siginfo.signals_mask = ALL_SIGNALS_BLOCKED;
    thrd->siginfo.pending_signals = PENDING_SIGNALS_CLEAR;

    thrd->refc = (ref_count) REF_COUNT_STATIC_INITIALIZER;
    thrd->kernel_thread = true;
    thrd->should_die = false;

    thrd->kernel_stack = kernel_stack;
    thrd->context = arch_kthread_context_init(thrd, func);
    thrd->state = INITIALISED;

    thrd->parent = NULL;
    thrd->proc = &kernel_process;

    if (!array_list_init(&thrd->children, 0))
        goto failed_to_init_child_list;

    thrd->finish_cvar = (con_var) CON_VAR_STATIC_INITIALIZER;
    thrd->lock = SPIN_LOCK_STATIC_INITIALIZER;
    thrd->scheduler_node = (linked_list_node) LINKED_LIST_NODE_OF(thrd);

    // Each kernel thread runs as separate thread group inside kernel process
    if (!process_add_thread(&kernel_process, (struct thread*) thrd))
        goto failed_to_add_thread_to_parent;

    bool interrupts_enabled = spin_lock_irq_save(&kernel_process.lock);
    ref_acquire(&kernel_process.refc);
    spin_unlock_irq_restore(&kernel_process.lock, interrupts_enabled);

    return true;

failed_to_add_thread_to_parent:
    array_list_deinit(&thrd->children);

failed_to_init_child_list:
    kfree(kernel_stack);

failed_to_allocate_kernel_stack:
    id_generator_free_id(&kernel_process.tgid_generator, thrd->tgid);

failed_to_allocate_tgid:
    threading_free_tid(thrd->id);

failed_to_allocate_tid:
    return false;
}

kthread* kthread_create(string name, kthread_func* func) {
    kthread* thrd = (kthread*) kmalloc(sizeof(kthread));
    if (!thrd)
        return NULL;

    if (!kthread_init(thrd, name, func)) {
        kfree(thrd);
        return NULL;
    }

    return thrd;
}

void kthread_start(kthread* thrd) { thread_start(thrd); }

kthread* kthread_run(string name, kthread_func* func) {
    kthread* thrd = kthread_create(name, func);
    if (thrd) {
        kthread_start(thrd);
    }

    return thrd;
}

void kthread_kill(kthread* thrd) {
    bool interrupts_enabled = spin_lock_irq_save(&thrd->lock);
    if (!thrd->exiting) {
        thrd->should_die = true;
    }
    spin_unlock_irq_restore(&thrd->lock, interrupts_enabled);
}

bool kthread_should_die() {
    kthread* current = get_current_thread();
    bool interrupts_enabled = spin_lock_irq_save(&current->lock);
    bool should_die = current->should_die;
    spin_unlock_irq_restore(&current->lock, interrupts_enabled);
    return should_die;
}