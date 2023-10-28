#include "../../../scheduler/scheduler.h"
#include "../../../spin_lock.h"

lock scheduler_lock;
static thread* current_thread = NULL;

void init_scheduler() { init_lock(&scheduler_lock); }

extern void _resume_thread();
void resume_thread(thread* thrd) {
    if (current_thread == NULL) {
        current_thread = thrd;
    }
    __asm__ volatile("movq %0, %%rsp\n"
                     "jmp _resume_thread"
                     :
                     : "r"(thrd->stack_pointer)
                     : "memory");
}

// this one will release scheduler_lock and enable interrupts
extern void _context_switch(u64* rsp_old, u64* rsp_new);

void switch_context(thread* thrd) {
    spin_lock_irq(&scheduler_lock);

    thread* old_thrd = current_thread;
    old_thrd->state = STOPPED;
    current_thread = thrd;

    thrd->state = RUNNING;
    _context_switch(&old_thrd->stack_pointer, &current_thread->stack_pointer);
}