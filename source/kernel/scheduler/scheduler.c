#include "scheduler.h"
#include "../arch/x86_64/cpu/tss.h"
#include "../idle.h"
#include "../lib/kprint.h"
#include "../threading/kthread.h"

static lock scheduler_lock = SPIN_LOCK_STATIC_INITIALIZER;
static queue run_queue = QUEUE_STATIC_INITIALIZER;

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
    bool interrupts_enabled = spin_lock_irq_save(&scheduler_lock);
    queue_push(&run_queue, &thrd->scheduler_node);
    spin_unlock_irq_restore(&scheduler_lock, interrupts_enabled);
}

void schedule_thread_exit() {
    bool interrupts_enabled = spin_lock_irq_save(&scheduler_lock);
    current_thread = NULL;
    spin_unlock_irq_restore(&scheduler_lock, interrupts_enabled);
}

// TODO: Add proper locking while accessing thread fields
struct cpu_context* context_switch(struct cpu_context* context) {
    bool interrupts_enabled = spin_lock_irq_save(&scheduler_lock);

    thread* old_thread = current_thread;
    if (old_thread) {
        old_thread->context = context;
    }

    queue_node* new_node = queue_pop(&run_queue);
    thread* new_thread = new_node ? (thread*) new_node->value : NULL;

    // we are in one and only alive running thread, just resume it
    if (!new_thread && old_thread && old_thread->state == RUNNING) {
        spin_unlock_irq_restore(&scheduler_lock, interrupts_enabled);
        return context;
    }

    // old thread can't continue running and no next thread, so just wake up
    // kernel wait thread
    current_thread = new_thread = new_thread ? new_thread : kernel_wait_thread;
    new_thread->state = RUNNING;

    if (old_thread && old_thread->state == RUNNING) {
        old_thread->state = STOPPED;

        // if we came from kernel wait thread then not add it to run queue
        if (old_thread != kernel_wait_thread) {
            queue_push(&run_queue, &old_thread->scheduler_node);
        }
    }

    struct cpu_context* new_context = new_thread->context;
    tss_update_rsp((u64) new_thread->kernel_stack + 4096);
    spin_unlock_irq_restore(&scheduler_lock, interrupts_enabled);

    return new_context;
}
