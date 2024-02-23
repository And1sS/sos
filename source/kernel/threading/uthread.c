#include "uthread.h"
#include "../arch/common/context.h"
#include "../arch/common/thread.h"
#include "../arch/common/vmm.h"
#include "../memory/heap/kheap.h"
#include "../memory/memory_map.h"
#include "../scheduler/scheduler.h"
#include "process.h"
#include "threading.h"

#define UTHREAD_CHILDREN_INITIAL_CAPACITY 8

bool uthread_init(process* proc, uthread* parent, uthread* thrd, string name,
                  void* stack, uthread_func* func) {

    memset(thrd, 0, sizeof(thread));
    bool allocated_tid = threading_allocate_tid(&thrd->id);
    if (!allocated_tid)
        goto failed_to_allocate_tid;

    void* kernel_stack = kmalloc_aligned(THREAD_KERNEL_STACK_SIZE, PAGE_SIZE);
    if (!kernel_stack)
        goto failed_to_allocate_kernel_stack;

    memset(kernel_stack, 0, THREAD_KERNEL_STACK_SIZE);
    memset(stack, 0, THREAD_KERNEL_STACK_SIZE);

    thrd->name = name;

    thrd->exiting = false;
    thrd->finished = false;
    thrd->exit_code = 0;

    memset(&thrd->siginfo, 0, sizeof(thread_siginfo));
    thrd->siginfo.signals_mask = ALL_SIGNALS_UNBLOCKED;
    thrd->siginfo.pending_signals = PENDING_SIGNALS_CLEAR;

    thrd->refc = (ref_count) REF_COUNT_STATIC_INITIALIZER;

    thrd->kernel_thread = false;
    thrd->should_die = false;

    thrd->kernel_stack = kernel_stack;
    thrd->user_stack = stack;

    thrd->context = arch_uthread_context_init(thrd, func);
    thrd->state = INITIALISED;

    thrd->parent = NULL;
    thrd->proc = proc;

    if (!array_list_init(&thrd->children, UTHREAD_CHILDREN_INITIAL_CAPACITY))
        goto failed_to_init_child_list;

    thrd->finish_cvar = (con_var) CON_VAR_STATIC_INITIALIZER;
    thrd->lock = SPIN_LOCK_STATIC_INITIALIZER;
    thrd->scheduler_node = (linked_list_node) LINKED_LIST_NODE_OF(thrd);

    if (parent && !thread_add_child(thrd))
        goto failed_to_add_thread_to_parent;

    if (!process_add_thread(proc, (struct thread*) thrd))
        goto failed_to_add_thread_to_process;

    bool interrupts_enabled = spin_lock_irq_save(&proc->lock);
    ref_acquire(&proc->refc);
    spin_unlock_irq_restore(&proc->lock, interrupts_enabled);

    return true;

failed_to_add_thread_to_process:
    if (parent) {
        thread_remove_child(thrd);
    }

failed_to_add_thread_to_parent:
    array_list_deinit(&thrd->children);

failed_to_init_child_list:
    kfree(kernel_stack);

failed_to_allocate_kernel_stack:
    threading_free_tid(thrd->id);

failed_to_allocate_tid:
    return false;
}

uthread* uthread_create_orphan(process* proc, string name, void* stack,
                               uthread_func* func) {

    // TODO: Add pointer checks, e.g. that stack and func is in user space
    uthread* thrd = (uthread*) kmalloc(sizeof(uthread));
    if (!thrd) {
        return false;
    }

    uthread_init(proc, NULL, thrd, name, stack, func);
    return thrd;
}

uthread* uthread_create(string name, void* stack, uthread_func* func) {
    uthread* current = get_current_thread();
    // TODO: Add pointer checks, e.g. that stack and func is in user space
    uthread* thrd = (uthread*) kmalloc(sizeof(uthread));
    if (!thrd) {
        return false;
    }

    uthread_init(current->proc, current, thrd, name, stack, func);
    return thrd;
}