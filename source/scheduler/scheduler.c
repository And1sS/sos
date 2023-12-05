#include "scheduler.h"
#include "../idle.h"
#include "../lib/container/queue/queue.h"
#include "../synchronization/spin_lock.h"
#include "../vga_print.h"

#define THREAD(node) ((thread*) node->value)
#define THREAD_STATE(node) THREAD(node)->state
#define THREAD_CONTEXT(node) THREAD(node)->context

static lock scheduler_lock;

static queue* run_queue;

static linked_list_node* current_thread_node;
static linked_list_node* kernel_wait_thread_node; // shouldn't enter run queue

_Noreturn void kernel_wait_thread_func() {
    while (true) {
        println("kernel wait thread!");
        halt();
    }
}

void scheduler_init() {
    init_lock(&scheduler_lock);

    current_thread_node = NULL;
    run_queue = queue_create();

    kernel_wait_thread_node = linked_list_node_create(
        thread_create("kernel-wait-thread", kernel_wait_thread_func));
}

linked_list_node* get_current_thread_node() {
    bool interrupts_enabled = spin_lock_irq_save(&scheduler_lock);
    linked_list_node* current_node = current_thread_node;
    spin_unlock_irq_restore(&scheduler_lock, interrupts_enabled);
    return current_node;
}

void schedule_thread_start(thread* thrd) {
    bool interrupts_enabled = spin_lock_irq_save(&scheduler_lock);
    queue_push(run_queue, queue_entry_create(thrd));
    spin_unlock_irq_restore(&scheduler_lock, interrupts_enabled);
}

void schedule_thread(linked_list_node* node) {
    bool interrupts_enabled = spin_lock_irq_save(&scheduler_lock);
    queue_push(run_queue, node);
    spin_unlock_irq_restore(&scheduler_lock, interrupts_enabled);
}

void schedule_thread_exit() {
    bool interrupts_enabled = spin_lock_irq_save(&scheduler_lock);
    // TODO: think about who is responsible for calling thread destruction
    thread_destroy(THREAD(current_thread_node));
    current_thread_node = NULL;
    spin_unlock_irq_restore(&scheduler_lock, interrupts_enabled);

    // We don't care about lost atomicity in this case, because
    // if timer interrupt comes in between, thread context will be switched and
    // never returned here. Other interrupts should not make a difference
    schedule();
}

void schedule_thread_block() {
    bool interrupts_enabled = spin_lock_irq_save(&scheduler_lock);
    THREAD_STATE(current_thread_node) = BLOCKED;
    spin_unlock_irq_restore(&scheduler_lock, interrupts_enabled);
}

struct cpu_context* context_switch(struct cpu_context* context) {
    bool interrupts_enabled = spin_lock_irq_save(&scheduler_lock);

    linked_list_node* old_node = current_thread_node;
    if (old_node) {
        THREAD_CONTEXT(old_node) = context;
    }

    linked_list_node* new_node = queue_pop(run_queue);

    // we are in one and only alive running thread, just resume it
    if (!new_node && old_node && THREAD_STATE(old_node) == RUNNING) {
        spin_unlock_irq_restore(&scheduler_lock, interrupts_enabled);
        return context;
    }

    // old thread can't continue running and no next thread, so just wake up
    // kernel wait thread
    new_node = new_node ? new_node : kernel_wait_thread_node;
    current_thread_node = new_node;
    THREAD_STATE(new_node) = RUNNING;

    if (old_node && THREAD_STATE(old_node) == RUNNING) {
        THREAD_STATE(old_node) = STOPPED;

        // if we came from kernel wait thread then not add it to run queue
        if (old_node != kernel_wait_thread_node) {
            queue_push(run_queue, old_node);
        }
    }

    spin_unlock_irq_restore(&scheduler_lock, interrupts_enabled);
    return THREAD_CONTEXT(new_node);
}
