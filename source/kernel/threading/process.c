#include "process.h"
#include "../lib/kprint.h"
#include "../memory/heap/kheap.h"
#include "../memory/virtual/vmm.h"
#include "threading.h"

bool process_init(process* proc, bool kernel_process) {
    if (!threading_allocate_pid(&proc->id))
        goto failed_to_allocate_pid;

    if (!array_list_init(&proc->thread_groups, 8))
        goto failed_to_init_thread_group_list;

    vm_space* kernel_vm = vmm_kernel_vm_space();
    proc->vm = kernel_process ? kernel_vm : vm_space_fork(kernel_vm);
    if (!proc->vm)
        goto failed_to_fork_vm_space;

    proc->cleaner_node = (linked_list_node) LINKED_LIST_NODE_OF(proc);

    proc->kernel_process = kernel_process;
    proc->lock = SPIN_LOCK_STATIC_INITIALIZER;
    proc->refc = (ref_count) REF_COUNT_STATIC_INITIALIZER;
    proc->finishing = false;

    return true;

failed_to_fork_vm_space:
    array_list_deinit(&proc->thread_groups);

failed_to_init_thread_group_list:
    threading_free_pid(proc->id);

failed_to_allocate_pid:
    return false;
}

void process_destroy(process* proc) {
    if (proc->kernel_process)
        panic("Trying to destroy kernel process");

    print("Destroying process!");

    vm_space_destroy(proc->vm);
    threading_free_pid(proc->id);
    kfree(proc);
}

bool process_add_thread_group(process* proc, struct thread* thrd) {
    bool interrupts_enabled = spin_lock_irq_save(&proc->lock);
    if (proc->finishing)
        panic("Trying to add new thread to finishing process");

    bool added = array_list_add_last(&proc->thread_groups, thrd);
    spin_unlock_irq_restore(&proc->lock, interrupts_enabled);

    return added;
}

bool process_exit_thread(process* proc, struct thread* thrd) {
    bool interrupts_enabled = spin_lock_irq_save(&proc->lock);
    ref_release(&proc->refc);

    // thread may be a part of thread group, in that case we won't remove it
    // from that list, this means that we still have running thread groups and
    // don't need to destroy process
    if (!array_list_remove(&proc->thread_groups, thrd)) {
        spin_unlock_irq_restore(&proc->lock, interrupts_enabled);
        return false;
    }

    // last leaving thread group should destroy process
    if (!proc->kernel_process && proc->thread_groups.size == 0) {
        proc->finishing = true;

        WAIT_FOR_IRQ(&proc->refc.empty_cvar, &proc->lock, interrupts_enabled,
                     proc->refc.count == 0);

        spin_unlock_irq_restore(&proc->lock, interrupts_enabled);
        return true;
    }

    spin_unlock_irq_restore(&proc->lock, interrupts_enabled);
    return false;
}

// void process_exit(u64 exit_code) {
//     thread* current = get_current_thread();
//     process* proc = current->proc;
//
//     bool interrupts_enabled = spin_lock_irq_save(&current->proc->lock);
//
//     // TODO: Pass child processes to init
// }