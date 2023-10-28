#include "../../../scheduler/scheduler.h"
#include "../../../interrupts/interrupts.h"
#include "../../../spin_lock.h"

static lock scheduler_lock;
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

extern void _context_switch(u64* rsp_old, u64* rsp_new);
void switch_context(thread* thrd) {
    disable_interrupts();

    thread* old_thrd = current_thread;
    old_thrd->state = STOPPED;
    current_thread = thrd;

    thrd->state = RUNNING;
    _context_switch(&old_thrd->stack_pointer, &current_thread->stack_pointer);
}