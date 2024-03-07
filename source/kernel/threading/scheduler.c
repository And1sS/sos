#include "scheduler.h"
#include "../idle.h"
#include "kthread.h"

static lock schedule_lock = SPIN_LOCK_STATIC_INITIALIZER;
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

void scheduler_lock() { spin_lock(&schedule_lock); }

void scheduler_unlock() { spin_unlock(&schedule_lock); }

thread* get_current_thread() {
    bool interrupts_enabled = spin_lock_irq_save(&schedule_lock);
    thread* current = current_thread;
    spin_unlock_irq_restore(&schedule_lock, interrupts_enabled);
    return current;
}

void schedule_thread(thread* thrd) {
    bool interrupts_enabled = spin_lock_irq_save(&schedule_lock);
    if (!thrd->currently_running && !thrd->on_run_queue) {
        queue_push(&run_queue, &thrd->scheduler_node);
        thrd->on_run_queue = true;
    }

    spin_unlock_irq_restore(&schedule_lock, interrupts_enabled);
}

void schedule_thread_exit() {
    bool interrupts_enabled = spin_lock_irq_save(&schedule_lock);
    current_thread = NULL;
    spin_unlock_irq_restore(&schedule_lock, interrupts_enabled);
}

// This should be called with scheduler lock held
struct cpu_context* context_switch(struct cpu_context* context) {
    thread* old_thread = current_thread;
    if (old_thread) {
        old_thread->context = context;
        old_thread->currently_running = false;
    }

    queue_node* new_node = queue_pop(&run_queue);
    thread* new_thread = new_node ? (thread*) new_node->value : NULL;

    // we are in one and only alive running thread, just resume it
    if (!new_thread && old_thread && old_thread->state == RUNNING) {
        old_thread->currently_running = true;
        return context;
    }

    if (old_thread && old_thread->state == RUNNING) {
        old_thread->state = STOPPED;

        // if we came from kernel wait thread then not add it to run queue
        if (old_thread != kernel_wait_thread) {
            queue_push(&run_queue, &old_thread->scheduler_node);
            old_thread->on_run_queue = true;
        }
    }

    // old thread can't continue running and no next thread, so just wake up
    // kernel wait thread
    current_thread = new_thread ? new_thread : kernel_wait_thread;
    current_thread->state = RUNNING;
    current_thread->on_run_queue = false;
    current_thread->currently_running = true;

    return current_thread->context;
}
