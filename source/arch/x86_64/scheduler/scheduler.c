#include "../../../scheduler/scheduler.h"
#include "../../../idle.h"
#include "../../../lib/container/array_list/array_list.h"
#include "../../../lib/container/queue/queue.h"
#include "../../../memory/heap/kheap.h"
#include "../../../spin_lock.h"
#include "../../../vga_print.h"

#define THREAD_VALUE(node) ((thread*) node->value)
#define THREAD_STATE(node) THREAD_VALUE(node)->state
#define THREAD_RSP(node) THREAD_VALUE(node)->stack_pointer

// this is used by context_switch.asm
lock scheduler_lock;

static array_list* thread_list;
static queue* run_queue;

static linked_list_node* current_thread_node;
static linked_list_node* kernel_wait_thread_node; // shouldn't enter run queue

void schedule_unsafe();

_Noreturn void kernel_wait_thread_func() {
    while (true) {
        println("kernel wait thread!");
        halt();
    }
}

void init_scheduler() {
    init_lock(&scheduler_lock);

    current_thread_node = NULL;

    thread_list = array_list_create();
    run_queue = queue_create();

    thread* kernel_wait_thread = (thread*) kmalloc(sizeof(thread));
    init_thread(kernel_wait_thread, "kernel-wait-thread",
                kernel_wait_thread_func);
    kernel_wait_thread_node = linked_list_node_create(kernel_wait_thread);
}

linked_list_node* get_current_thread_node() {
    bool interrupts_enabled = spin_lock_irq_save(&scheduler_lock);
    linked_list_node* current_node = current_thread_node;
    spin_unlock_irq_restore(&scheduler_lock, interrupts_enabled);
    return current_node;
}

void schedule_thread_start(thread* thrd) {
    bool interrupts_enabled = spin_lock_irq_save(&scheduler_lock);
    array_list_add_last(thread_list, thrd);
    queue_push(run_queue, queue_entry_create(thrd));
    spin_unlock_irq_restore(&scheduler_lock, interrupts_enabled);
}

void schedule_thread(linked_list_node* node) {
    bool interrupts_enabled = spin_lock_irq_save(&scheduler_lock);
    queue_push(run_queue, node);
    spin_unlock_irq_restore(&scheduler_lock, interrupts_enabled);
}

void schedule_thread_exit() {
    spin_lock_irq(&scheduler_lock);
    THREAD_STATE(current_thread_node) = DEAD;
    kfree(THREAD_VALUE(current_thread_node)->stack);
    kfree(current_thread_node);
    current_thread_node = NULL;

    schedule_unsafe();
}

void schedule_thread_block() {
    bool interrupts_enabled = spin_lock_irq_save(&scheduler_lock);
    THREAD_STATE(current_thread_node) = BLOCKED;
    spin_unlock_irq_restore(&scheduler_lock, interrupts_enabled);
}

void schedule() {
    spin_lock_irq(&scheduler_lock);
    schedule_unsafe();
}

// this one will release scheduler_lock and enable interrupts
extern void _context_switch(u64* rsp_old, u64* rsp_new);

// assumes that scheduler lock is held
void schedule_unsafe() {
    linked_list_node* old_node = current_thread_node;
    linked_list_node* new_node = queue_pop(run_queue);

    // we are in one and only alive running thread, just resume it
    if (!new_node && old_node && THREAD_STATE(old_node) == RUNNING) {
        spin_unlock_irq(&scheduler_lock);
        return;
    }

    // old thread can't continue running and no next thread, so just wake up
    // kernel wait thread
    new_node = new_node ? new_node : kernel_wait_thread_node;
    current_thread_node = new_node;
    THREAD_STATE(new_node) = RUNNING;

    // dead thread does not need saved context, so just resume next thread
    if (old_node) {
        if (THREAD_STATE(old_node) == RUNNING) {
            THREAD_STATE(old_node) = STOPPED;

            // if we came from kernel wait thread then not add it to run queue
            if (old_node != kernel_wait_thread_node) {
                queue_push(run_queue, old_node);
            }
        }

        // no need to release the lock and enable interrupts here, since
        // _context_switch will do it
        _context_switch(&THREAD_RSP(old_node), &THREAD_RSP(new_node));
    } else {
        // no old thread - just resume next thread. only release the lock,
        // interrupts will be enabled by _resume_thread
        spin_unlock(&scheduler_lock);
        __asm__ volatile(
            "movq %0, %%rsp\n"
            "jmp _resume_thread" // this label is defined in context_switch.asm
            :
            : "r"(THREAD_RSP(new_node))
            : "memory");
    }
}