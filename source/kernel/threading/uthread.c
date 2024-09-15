#include "uthread.h"
#include "../arch/common/thread.h"
#include "process.h"
#include "scheduler.h"

#define UTHREAD_CHILDREN_INITIAL_CAPACITY 8

#define UTHREAD_MAX_STACKS 4096

const vm_area_flags USER_STACK_FLAGS = {
    .writable = true, .executable = true, .user_access_allowed = true};

static void* uthread_map_user_stack(process* proc, u64 tgid);

static bool uthread_init(process* proc, uthread* parent, uthread* thrd,
                         string name, void* user_stack, uthread_func* func) {

    memset(thrd, 0, sizeof(thread));
    if (!threading_allocate_tid(&thrd->id))
        goto failed_to_allocate_tid;

    if (!id_generator_get_id(&proc->tgid_generator, &thrd->tgid))
        goto failed_to_allocate_tgid;

    thrd->kernel_stack = kmalloc_aligned(THREAD_KERNEL_STACK_SIZE, PAGE_SIZE);
    if (!thrd->kernel_stack)
        goto failed_to_allocate_kernel_stack;
    memset(thrd->kernel_stack, 0, THREAD_KERNEL_STACK_SIZE);

    thrd->user_stack =
        user_stack ? user_stack : uthread_map_user_stack(proc, thrd->tgid);
    if (!thrd->user_stack)
        goto failed_to_map_user_stack;

    thrd->name = name;

    thrd->exiting = false;
    thrd->finished = false;
    thrd->exit_code = 0;

    memset(&thrd->siginfo, 0, sizeof(thread_siginfo));
    thrd->siginfo_lock = SPIN_LOCK_STATIC_INITIALIZER;
    thrd->siginfo.signals_mask = ALL_SIGNALS_UNBLOCKED;
    thrd->siginfo.pending_signals = PENDING_SIGNALS_CLEAR;

    thrd->refc = (ref_count) REF_COUNT_STATIC_INITIALIZER;

    thrd->kernel_thread = false;
    thrd->should_die = false;

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
    if (parent)
        thread_remove_child(thrd);

failed_to_add_thread_to_parent:
    array_list_deinit(&thrd->children);

failed_to_init_child_list:
    // TODO: Unmap mapped user stack

failed_to_map_user_stack:
    kfree(thrd->kernel_stack);

failed_to_allocate_kernel_stack:
    id_generator_free_id(&proc->tgid_generator, thrd->tgid);

failed_to_allocate_tgid:
    threading_free_tid(thrd->id);

failed_to_allocate_tid:
    return false;
}

/*
 * User thread stacks are allocated from top of user space to bottom.
 * allocation direction <-- | unused | thread n stack | .. | thread 0 stack |
 *
 * If, for some reason, corresponding stack pages are already mapped inside user
 * space, try to find next unmapped place.
 */
static void* uthread_map_user_stack(process* proc, u64 tgid) {
    void* stack = NULL;

    rw_spin_lock_write_irq(&proc->vm->lock);
    for (u64 i = tgid; i < UTHREAD_MAX_STACKS; i++) {
        u64 addr = (USER_SPACE_END_VADDR + 1) - (i + 1) * USER_STACK_SIZE;
        vm_page_mapping_result result = vm_space_map_pages_exactly(
            proc->vm, addr, USER_STACK_PAGE_COUNT, USER_STACK_FLAGS);

        switch (result) {
        case SUCCESS:
            stack = (void*) addr;
            goto exit_loop;

        case ALREADY_MAPPED:
            continue;

        case UNAUTHORIZED:
        case INVALID_RANGE:
            panic("Invalid range inside user thread stack mapping");

        case OUT_OF_MEMORY:
            goto exit_loop;
        }
    }

exit_loop:
    rw_spin_unlock_write_irq(&proc->vm->lock);

    return stack;
}

uthread* uthread_create_orphan(process* proc, string name, void* user_stack,
                               uthread_func* func) {

    uthread* thrd = (uthread*) kmalloc(sizeof(uthread));
    if (!thrd)
        return NULL;

    if (!uthread_init(proc, NULL, thrd, name, user_stack, func)) {
        kfree(thrd);
        return NULL;
    }

    return thrd;
}

uthread* uthread_create(string name, uthread_func* func) {
    uthread* current = get_current_thread();
    uthread* thrd = (uthread*) kmalloc(sizeof(uthread));
    if (!thrd)
        return NULL;

    if (!uthread_init(current->proc, current, thrd, name, NULL, func)) {
        kfree(thrd);
        return NULL;
    }

    return thrd;
}