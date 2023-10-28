#include "../../../scheduler/scheduler.h"
#include "../../../spin_lock.h"
#include "../../../vga_print.h"

lock scheduler_lock;

static thread* threads[10];
static u64 thread_count = 0;
static u64 cur_thread_id = 0;

thread kernel_thread;
_Noreturn void kernel_thread_func() {
    while (true) {
        println("KERNEL THREAD!");
    }
}

void init_scheduler() {
    init_lock(&scheduler_lock);

    init_thread(&kernel_thread, "kernel-thread", kernel_thread_func);
    add_thread(&kernel_thread);
}

void schedule_thread_exit() {
}

extern void _resume_thread();
void resume_thread(thread* thrd) {
    cur_thread_id = thrd->id;
    __asm__ volatile("movq %0, %%rsp\n"
                     "jmp _resume_thread"
                     :
                     : "r"(thrd->stack_pointer)
                     : "memory");
}

// this one will release scheduler_lock and enable interrupts
extern void _context_switch(u64* rsp_old, u64* rsp_new);
void add_thread(thread* thrd) {
    threads[thread_count++] = thrd;
}

void switch_context() {
    spin_lock_irq(&scheduler_lock);
    if (thread_count <= 1) {
        spin_unlock_irq(&scheduler_lock);
        return;
    }

    thread* old_thrd = threads[cur_thread_id];
    old_thrd->state = STOPPED;

    cur_thread_id = (cur_thread_id + 1) % thread_count;
    thread* current_thread = threads[cur_thread_id];
    current_thread->state = RUNNING;

    _context_switch(&old_thrd->stack_pointer, &current_thread->stack_pointer);
}