#include "scheduler.h"
#include "../lib/memory_util.h"
#include "../memory/heap/kheap.h"
#include "../memory/memory_map.h"
#include "../spin_lock.h"

lock scheduler_lock;

u64 thread_seq = 0;
thread* current_thread = NULL;

void init_scheduler() { init_lock(&scheduler_lock); }

void thread_exit() {}

void kernel_thread_wrapper(thread_func* func) {
    func();
    thread_exit();
}

bool init_thread(thread* thrd, string name, thread_func* func) {
    bool interrupts_enabled = spin_lock_irq_save(&scheduler_lock);
    memset(thrd, 0, sizeof(thread));

    thrd->id = thread_seq++;

    void* stack = kmalloc_aligned(THREAD_STACK_SIZE, FRAME_SIZE);
    if (stack == NULL) {
        spin_unlock_irq_restore(&scheduler_lock, interrupts_enabled);
        return false;
    }
    memset(stack, 0, THREAD_STACK_SIZE);

    thrd->stack = stack;
    thrd->name = name;
    thrd->state = INITIALISED;
    thrd->rsp = (u64) stack + THREAD_STACK_SIZE - sizeof(thread_context);

    thread_context* context = (thread_context*) thrd->rsp;
    memset(context, 0, sizeof(thread_context));

    context->kernel_wrapper_func = func;
    context->init_func = func;

    spin_unlock_irq_restore(&scheduler_lock, interrupts_enabled);
    return true;
}

void switch_context(thread* thrd) {
    bool interrupts_enabled = spin_lock_irq_save(&scheduler_lock);
    if (current_thread != NULL) {
    }

    current_thread = thrd;

    thrd->state = RUNNING;
    spin_unlock_irq_restore(&scheduler_lock, interrupts_enabled);
}