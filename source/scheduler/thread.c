#include "thread.h"
#include "../id_generator.h"
#include "../lib/memory_util.h"
#include "../memory/heap/kheap.h"
#include "../memory/memory_map.h"
#include "../vga_print.h"
#include "scheduler.h"

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

    thrd->stack = stack;
    thrd->name = name;
    thrd->state = INITIALISED;
    thrd->context = arch_thread_context_init(thrd, func);

    return true;
}

thread* thread_create(string name, thread_func* func) {
    thread* thrd = (thread*) kmalloc(sizeof(thread));
    if (!thrd)
        return NULL;

    thread_init(thrd, name, func);
    return thrd;
}

void thread_destroy(thread* thrd) {
    id_generator_free_id(&id_gen, thrd->id);
    kfree(thrd->stack);
    kfree(thrd);
}

void thread_start(thread* thrd) {
    schedule_thread_start(thrd);
}
