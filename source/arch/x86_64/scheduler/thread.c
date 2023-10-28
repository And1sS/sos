#include "../../../scheduler/thread.h"
#include "../../../id_generator.h"
#include "../../../lib/memory_util.h"
#include "../../../memory/heap/kheap.h"
#include "../../../memory/memory_map.h"

typedef struct __attribute__((__packed__)) {
    u64 rax;
    u64 rbx;
    u64 rcx;
    u64 rdx;
    u64 rsi;
    u64 rdi;
    u64 rbp;
    u64 r8;
    u64 r9;
    u64 r10;
    u64 r11;
    u64 r12;
    u64 r13;
    u64 r14;

    void* kernel_wrapper_func;
    void* unused_return_addr;
} thread_context;

static id_generator id_gen;

void init_thread_creator() { init_id_generator(&id_gen); }

void thread_exit() {}

void kernel_thread_wrapper(thread_func* func) {
    func();
    thread_exit();
}

thread* init_thread(thread* thrd, string name, thread_func* func) {
    if (thrd == NULL) {
        thrd = kmalloc(sizeof(thrd));
    }

    if (thrd == NULL) {
        return NULL;
    }
    memset(thrd, 0, sizeof(thread));

    thrd->id = get_id(&id_gen);

    void* stack = kmalloc_aligned(THREAD_STACK_SIZE, FRAME_SIZE);
    if (stack == NULL) {
        return NULL;
    }
    memset(stack, 0, THREAD_STACK_SIZE);

    thrd->stack = stack;
    thrd->name = name;
    thrd->state = INITIALISED;
    thrd->stack_pointer =
        (u64) stack + THREAD_STACK_SIZE - sizeof(thread_context);

    thread_context* context = (thread_context*) thrd->stack_pointer;
    memset(context, 0, sizeof(thread_context));

    context->kernel_wrapper_func = kernel_thread_wrapper;
    context->rdi = (u64) func;

    return thrd;
}
