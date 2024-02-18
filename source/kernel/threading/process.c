#include "process.h"
#include "../interrupts/irq.h"
#include "../lib/kprint.h"
#include "../memory/virtual/vmm.h"

bool process_init(process* proc, bool kernel_process) {
    if (!array_list_init(&proc->thread_groups, 8)) {
        return false;
    }

    vm_space* kernel_vm = vmm_kernel_vm_space();
    proc->vm = kernel_process ? kernel_vm : vm_space_fork(kernel_vm);

    proc->kernel_process = kernel_process;
    proc->lock = SPIN_LOCK_STATIC_INITIALIZER;
    proc->finishing = false;

    return true;
}

static void process_destroy(process* proc) {
    if (proc->kernel_process)
        panic("Trying to destroy kernel process");

    vm_space_destroy(proc->vm);
    print("Destroying process!");
}

bool process_add_thread_group(process* proc, struct thread* thrd) {
    bool interrupts_enabled = spin_lock_irq_save(&proc->lock);
    if (proc->finishing)
        panic("Trying to add new thread to finishing process");

    bool added = array_list_add_last(&proc->thread_groups, thrd);
    if (added)
        ref_acquire(&proc->refc);
    spin_unlock_irq_restore(&proc->lock, interrupts_enabled);

    return added;
}

void process_exit_thread(process* proc, struct thread* thrd) {
    bool interrupts_enabled = spin_lock_irq_save(&proc->lock);
    ref_release(&proc->refc);

    // thread may be a part of thread group, in that case we won't remove it
    // from that list, this means that we still have running thread groups and
    // don't need to destroy process
    if (!array_list_remove(&proc->thread_groups, thrd)) {
        spin_unlock_irq_restore(&proc->lock, interrupts_enabled);
        return;
    }

    // last leaving thread group should destroy process
    if (!proc->kernel_process && proc->thread_groups.size == 0) {
        proc->finishing = true;

        WAIT_FOR_IRQ(&proc->refc.empty_cvar, &proc->lock, interrupts_enabled,
                     proc->refc.count == 0);

        process_destroy(proc);
        local_irq_restore(interrupts_enabled);
        return;
    }

    spin_unlock_irq_restore(&proc->lock, interrupts_enabled);
}