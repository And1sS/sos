#include "uthread.h"
#include "../arch/common/context.h"
#include "../arch/common/thread.h"
#include "../arch/common/vmm.h"
#include "../memory/heap/kheap.h"
#include "../memory/memory_map.h"
#include "../scheduler/scheduler.h"
#include "threading.h"

#define UTHREAD_CHILDREN_INITIAL_CAPACITY 8

bool uthread_init(uthread* parent, uthread* thrd, string name, void* stack,
                  uthread_func* func) {

    memset(thrd, 0, sizeof(thread));
    bool allocated_tid = threading_allocate_tid(&thrd->id);
    if (!allocated_tid)
        return false;

    void* kernel_stack = kmalloc_aligned(THREAD_KERNEL_STACK_SIZE, PAGE_SIZE);

    if (!kernel_stack) {
        threading_free_tid(thrd->id);
        return false;
    }
    memset(kernel_stack, 0, THREAD_KERNEL_STACK_SIZE);
    memset(stack, 0, THREAD_KERNEL_STACK_SIZE);

    thrd->name = name;

    thrd->exiting = false;
    thrd->finished = false;
    thrd->exit_code = 0;

    thrd->signal_info.signals_mask = ALL_SIGNALS_UNBLOCKED;
    thrd->signal_info.pending_signals = PENDING_SIGNALS_CLEAR;
    memset(thrd->signal_info.signal_actions, 0,
           sizeof(sigaction) * (SIGNALS_COUNT + 1));

    thrd->refc = (ref_count) REF_COUNT_STATIC_INITIALIZER;

    thrd->kernel_thread = false;
    thrd->should_die = false;

    thrd->kernel_stack = kernel_stack;
    thrd->user_stack = stack;
    thrd->context = arch_uthread_context_init(thrd, func);
    thrd->state = INITIALISED;

    thrd->parent = NULL;

    // TODO: think of making scope of holding parent lock bigger when processes
    //       or signals added, since it will ease cleanups
    if (parent) {
        bool interrupts_enabled = spin_lock_irq_save(&parent->lock);
        array_list_add_last(&parent->children, thrd);
        thrd->parent = parent;
        ref_acquire(&thrd->refc);
        ref_acquire(&parent->refc);
        spin_unlock_irq_restore(&parent->lock, interrupts_enabled);
    }
    array_list_init(&thrd->children, UTHREAD_CHILDREN_INITIAL_CAPACITY);

    thrd->finish_cvar = (con_var) CON_VAR_STATIC_INITIALIZER;

    thrd->lock = SPIN_LOCK_STATIC_INITIALIZER;

    thrd->scheduler_node = (linked_list_node) LINKED_LIST_NODE_OF(thrd);

    return true;
}

uthread* uthread_create_orphan(string name, void* stack, uthread_func* func) {
    // TODO: Add pointer checks, e.g. that stack and func is in user space
    uthread* thrd = (uthread*) kmalloc(sizeof(uthread));
    if (!thrd) {
        return false;
    }

    uthread_init(NULL, thrd, name, stack, func);
    return thrd;
}

uthread* uthread_create(string name, void* stack, uthread_func* func) {
    uthread* current = get_current_thread();
    // TODO: Add pointer checks, e.g. that stack and func is in user space
    uthread* thrd = (uthread*) kmalloc(sizeof(uthread));
    if (!thrd) {
        return false;
    }

    uthread_init(current, thrd, name, stack, func);
    return thrd;
}