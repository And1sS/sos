#include "../../../scheduler/scheduler.h"
#include "../../../idle.h"
#include "../../../lib/container/linked_list/linked_list.h"
#include "../../../lib/container/queue/queue.h"
#include "../../../memory/heap/kheap.h"
#include "../../../spin_lock.h"
#include "../../../vga_print.h"

// this is used by context_switch.asm
lock scheduler_lock;

static bool started;

// TODO: rewrite to implementations that use array under the hood,
//       since linked list requires too much heap allocations
static linked_list* thread_list;
static queue* run_queue;

static thread* current_thread;

// shouldn't be present on run queue
static thread* kernel_wait_thread;

// this one will release scheduler_lock and enable interrupts
extern void _context_switch(u64* rsp_old, u64* rsp_new);

void start_thread_unsafe(thread* thrd);
void switch_context_unsafe();

_Noreturn void kernel_wait_thread_func() {
    while (true) {
        println("kernel wait thread!");
        halt();
    }
}

void init_scheduler() {
    init_lock(&scheduler_lock);

    started = false;
    current_thread = NULL;

    thread_list = linked_list_create();
    run_queue = queue_create();

    kernel_wait_thread = (thread*) kmalloc(sizeof(thread));
    init_thread(kernel_wait_thread, "kernel-wait-thread",
                kernel_wait_thread_func);
}

void start_scheduler() {
    bool interrupts_enabled = spin_lock_irq_save(&scheduler_lock);
    started = true;
    spin_unlock_irq_restore(&scheduler_lock, interrupts_enabled);
}

void schedule_thread_exit() {
    spin_lock_irq(&scheduler_lock);
    current_thread->state = DEAD;
    kfree(current_thread->stack);
    switch_context_unsafe();
}

// this one will enable interrupts
void resume_thread(thread* thrd) {
    __asm__ volatile(
        "movq %0, %%rsp\n"
        "jmp _resume_thread" // this label is defined in context_switch.asm
        :
        : "r"(thrd->stack_pointer)
        : "memory");
}

void start_thread(thread* thrd) {
    bool interrupts_enabled = spin_lock_irq_save(&scheduler_lock);
    start_thread_unsafe(thrd);
    spin_unlock_irq_restore(&scheduler_lock, interrupts_enabled);
}

void start_thread_unsafe(thread* thrd) {
    linked_list_add_last(thread_list, thrd);
    queue_push(run_queue, thrd);
}

void switch_context() {
    spin_lock_irq(&scheduler_lock);
    switch_context_unsafe();
}

// assumes that scheduler lock is held
void switch_context_unsafe() {
    if (!started) {
        spin_unlock_irq(&scheduler_lock);
        return;
    }

    thread* old_thread = current_thread;
    thread* new_thread = queue_pop(run_queue);

    // we are in one and only alive running thread, just resume it
    if (!new_thread && old_thread && old_thread->state == RUNNING) {
        spin_unlock_irq(&scheduler_lock);
        return;
    }

    // we came from last alive running thread, which can't continue running for
    // some reason, so just wake up kernel wait thread
    if (!new_thread && old_thread && old_thread->state != RUNNING) {
        new_thread = kernel_wait_thread;
    }

    // now we have next thread to run for sure
    current_thread = new_thread;
    new_thread->state = RUNNING;

    // dead threads does not need saved context, so just resume next thread
    if (old_thread && old_thread->state != DEAD) {
        // if we came from kernel wait thread or a thread, that is not running
        // anymore, then not add it to run queue
        if (old_thread != kernel_wait_thread && old_thread->state == RUNNING) {
            queue_push(run_queue, old_thread);
        }

        if (old_thread->state == RUNNING) {
            old_thread->state = STOPPED;
        }

        // no need to release the lock and enable interrupts here, since
        // _context_switch will do it
        _context_switch(&old_thread->stack_pointer, &new_thread->stack_pointer);
    } else {
        // at the beginning of scheduling just release the lock, interrupts
        // will be enabled by resume_thread
        spin_unlock(&scheduler_lock);
        resume_thread(new_thread);
    }
}