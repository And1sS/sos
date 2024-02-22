#include "scheduler.h"
#include "../idle.h"
#include "../interrupts/irq.h"
#include "../memory/virtual/vmm.h"
#include "../threading/kthread.h"

static lock scheduler_lock = SPIN_LOCK_STATIC_INITIALIZER;
static queue run_queue = QUEUE_STATIC_INITIALIZER;

// TODO: make these per-cpu
static thread* current_thread = NULL;
static kthread* kernel_wait_thread; // shouldn't enter run queue

_Noreturn void kernel_wait_thread_func() {
    while (true) {
        halt();
    }
}

void scheduler_init() {
    kernel_wait_thread =
        kthread_create("kernel-wait-thread", kernel_wait_thread_func);
}

thread* get_current_thread() {
    bool interrupts_enabled = spin_lock_irq_save(&scheduler_lock);
    thread* current = current_thread;
    spin_unlock_irq_restore(&scheduler_lock, interrupts_enabled);
    return current;
}

void schedule_thread(thread* thrd) {
    bool interrupts_enabled = spin_lock_irq_save(&thrd->lock);
    if (thrd->on_scheduler_queue || thrd->state == RUNNING) {
        spin_unlock_irq_restore(&thrd->lock, interrupts_enabled);
        return;
    }

    thrd->on_scheduler_queue = true;
    spin_unlock(&thrd->lock);

    spin_lock(&scheduler_lock);
    queue_push(&run_queue, &thrd->scheduler_node);
    spin_unlock_irq_restore(&scheduler_lock, interrupts_enabled);
}

void schedule_thread_exit() {
    bool interrupts_enabled = spin_lock_irq_save(&scheduler_lock);
    current_thread = NULL;
    spin_unlock_irq_restore(&scheduler_lock, interrupts_enabled);
}

struct cpu_context* context_switch(struct cpu_context* context) {
    bool interrupts_enabled = spin_lock_irq_save(&scheduler_lock);
    thread* old_thread = current_thread;
    queue_node* new_node = queue_pop(&run_queue);
    thread* new_thread = new_node ? (thread*) new_node->value : NULL;
    spin_unlock(&scheduler_lock);

    bool push_old_thread = false;
    if (old_thread) {
        old_thread->context = context; // no locking required

        spin_lock(&old_thread->lock);
        // we are in one and only alive running thread, just resume it
        if (!new_thread && old_thread->state == RUNNING) {
            spin_unlock_irq_restore(&old_thread->lock, interrupts_enabled);
            return context;
        } else if (old_thread->state == RUNNING) {
            old_thread->state = STOPPED;
            old_thread->on_scheduler_queue = push_old_thread =
                old_thread != kernel_wait_thread;
        }
        spin_unlock(&old_thread->lock);
    }

    spin_lock(&scheduler_lock);
    // if we came from kernel wait thread then not add it to run queue
    if (push_old_thread) {
        queue_push(&run_queue, &old_thread->scheduler_node);
    }
    // old thread can't continue running and no next thread, so just wake up
    // kernel wait thread
    current_thread = new_thread = new_thread ? new_thread : kernel_wait_thread;
    spin_unlock(&scheduler_lock);

    spin_lock(&new_thread->lock);
    new_thread->on_scheduler_queue = false;
    new_thread->state = RUNNING;
    spin_unlock(&new_thread->lock);

    vmm_set_vm_space(new_thread->proc->vm);
    local_irq_restore(interrupts_enabled);

    return new_thread->context;
}
