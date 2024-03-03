#include "uprocess.h"
#include "../../arch/x86_64/cpu/cpu_context.h"
#include "../../error/errno.h"
#include "../../scheduler/scheduler.h"
#include "../thread/uthread.h"

uprocess* uprocess_create() {
    process* proc = kmalloc(sizeof(process));
    if (!proc)
        return NULL;

    if (!process_init(proc, false)) {
        kfree(proc);
        return NULL;
    }

    return proc;
}

u64 uprocess_fork(struct cpu_context* context) {
    thread* current = get_current_thread();
    process* proc = current->proc;

    process* created = uprocess_create();
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
    memcpy(&created->siginfo.dispositions, &proc->siginfo.dispositions,
           sizeof(signal_disposition) * (SIGNALS_COUNT + 1));
    spin_unlock_irq_restore(&proc->lock, interrupts_enabled);

    if (!process_add_child(created))
        goto failed_to_add_process_to_parent;

    cpu_context* forked_arch_context = (cpu_context*) start_thread->context;
    *forked_arch_context = *current_arch_context;
    forked_arch_context->rax = 0;

    thread_start(start_thread);

    return created->id;

failed_to_add_process_to_parent:
    process_destroy(created);

failed_to_create_start_process_thread:
failed_to_set_created_process_start_thread_tgid:
failed_to_create_process:
    return -ENOMEM;
}