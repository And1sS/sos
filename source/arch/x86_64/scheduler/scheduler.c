#include "../../../scheduler/scheduler.h"
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
static thread* kernel_thread;

// this one will release scheduler_lock and enable interrupts
extern void _context_switch(u64* rsp_old, u64* rsp_new);

void start_thread_unsafe(thread* thrd);

_Noreturn void kernel_thread_func() {
    while (true) {
        println("KERNEL THREAD!");
    }
}

void init_scheduler() {
    init_lock(&scheduler_lock);

    started = false;
    current_thread = NULL;

    thread_list = linked_list_create();
    run_queue = queue_create();

    kernel_thread = (thread*) kmalloc(sizeof(thread));
    init_thread(kernel_thread, "kernel-thread", kernel_thread_func);
}

void start_scheduler() {
    spin_lock_irq(&scheduler_lock);

    started = true;
    start_thread_unsafe(kernel_thread);

    spin_unlock_irq(&scheduler_lock);
}

void schedule_thread_exit() {}

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
    if (!started) {
        spin_unlock_irq(&scheduler_lock);
        return;
    }

    thread* old_thread = current_thread;
    thread* new_thread = queue_pop(run_queue);

    if (!new_thread) {
        // at least one kernel thread will be running, so if run queue is empty,
        // then we are running inside kernel waiting thread, so just continue
        // waiting
        spin_unlock_irq(&scheduler_lock);
        return;
    }

    current_thread = new_thread;
    new_thread->state = RUNNING;

    if (old_thread) {
        queue_push(run_queue, old_thread);
        old_thread->state = STOPPED;

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