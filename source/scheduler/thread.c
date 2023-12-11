#include "thread.h"
#include "../id_generator.h"
#include "../lib/memory_util.h"
#include "../memory/heap/kheap.h"
#include "../memory/memory_map.h"
#include "scheduler.h"
#include "thread_cleaner.h"

static id_generator id_gen;

extern struct cpu_context* arch_thread_context_init(thread* thrd,
                                                    thread_func* func);

void threading_init() { id_generator_init(&id_gen); }

bool thread_init(thread* thrd, string name, thread_func* func) {
    memset(thrd, 0, sizeof(thread));
    thrd->id = id_generator_get_id(&id_gen);

    void* stack = kmalloc_aligned(THREAD_STACK_SIZE, FRAME_SIZE);
    if (stack == NULL) {
        return false;
    }
    memset(stack, 0, THREAD_STACK_SIZE);

    thrd->lock = SPIN_LOCK_STATIC_INITIALIZER;
    thrd->stack = stack;
    thrd->name = name;
    thrd->state = INITIALISED;
    thrd->context = arch_thread_context_init(thrd, func);
    thrd->scheduler_node = (linked_list_node) LINKED_LIST_NODE_OF(thrd);

    return true;
}

thread* thread_create(string name, thread_func* func) {
    thread* thrd = (thread*) kmalloc(sizeof(thread));
    if (!thrd)
        return NULL;

    thread_init(thrd, name, func);
    return thrd;
}

thread* thread_run(string name, thread_func* func) {
    thread* thrd = thread_create(name, func);
    if (thrd) {
        thread_start(thrd);
    }

    return thrd;
}

void thread_start(thread* thrd) { schedule_thread(thrd); }

void thread_exit() {
    thread* current = get_current_thread();
    bool interrupts_enabled = spin_lock_irq_save(&current->lock);
    schedule_thread_exit();
    thread_cleaner_mark(current);
    spin_unlock_irq_restore(&current->lock, interrupts_enabled);

    schedule();
}

void thread_destroy(thread* thrd) {
    id_generator_free_id(&id_gen, thrd->id);
    kfree(thrd->stack);
    kfree(thrd);
}
