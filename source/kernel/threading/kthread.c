#include "kthread.h"
#include "../arch/common/thread.h"
#include "../arch/common/vmm.h"
#include "../lib/id_generator.h"
#include "../memory/heap/kheap.h"
#include "../memory/physical/pmm.h"
#include "scheduler.h"
#include "threading.h"

bool kthread_init(kthread* thrd, string name, kthread_func* func) {
    memset(thrd, 0, sizeof(thread));
    if (!threading_allocate_tid(&thrd->id))
        goto failed_to_allocate_tid;

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
    thrd->signal_info.signals_mask = ALL_SIGNALS_BLOCKED;
    thrd->signal_info.pending_signals = PENDING_SIGNALS_CLEAR;
    memset(thrd->signal_info.signal_actions, 0,
           sizeof(sigaction) * (SIGNALS_COUNT + 1));

    thrd->refc = (ref_count) REF_COUNT_STATIC_INITIALIZER;
    thrd->kernel_thread = true;
    thrd->should_die = false;

    thrd->kernel_stack = kernel_stack;
    thrd->context = arch_kthread_context_init(thrd, func);
    thrd->state = INITIALISED;
    thrd->currently_running = false;
    thrd->on_run_queue = false;

    thrd->parent = NULL;

    if (!array_list_init(&thrd->children, 0))
        goto failed_to_init_child_list;

    thrd->finish_cvar = (con_var) CON_VAR_STATIC_INITIALIZER;
    thrd->lock = SPIN_LOCK_STATIC_INITIALIZER;
    thrd->scheduler_node = (linked_list_node) LINKED_LIST_NODE_OF(thrd);

    return true;

failed_to_init_child_list:
    kfree(kernel_stack);

failed_to_allocate_kernel_stack:
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