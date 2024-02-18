#include "../interrupts/irq.h"
#include "../lib/kprint.h"
#include "../synchronization/con_var.h"
#include "kthread.h"
#include "thread.h"

static lock dead_lock = SPIN_LOCK_STATIC_INITIALIZER;
static con_var dead_cvar = CON_VAR_STATIC_INITIALIZER;

static linked_list dead_list = LINKED_LIST_STATIC_INITIALIZER;

_Noreturn void thread_cleaner_daemon();

void thread_cleaner_init() {
    kthread_run("kernel-thread-cleaner-daemon", thread_cleaner_daemon);
}

_Noreturn void thread_cleaner_daemon() {
    while (true) {
        bool interrupts_enabled = spin_lock_irq_save(&dead_lock);
        WAIT_FOR_IRQ(&dead_cvar, &dead_lock, interrupts_enabled,
                     dead_list.size != 0);

        linked_list_node* cur = linked_list_remove_first_node(&dead_list);
        while (cur) {
            thread* thrd = (thread*) cur->value;

            // Do not destroy thread until its lock is available
            spin_lock(&thrd->lock);

            print("destroying thread: ");
            print_u64(thrd->id);

            thread_destroy(thrd);
            cur = linked_list_remove_first_node(&dead_list);
        }

        spin_unlock_irq_restore(&dead_lock, interrupts_enabled);
    }
}

void thread_cleaner_mark(thread* thrd) {
    bool interrupts_enabled = spin_lock_irq_save(&dead_lock);
    // at this point thread will never be scheduled anymore, so we can utilize
    // its node for our purposes
    linked_list_add_last_node(&dead_list, &thrd->scheduler_node);
    con_var_broadcast(&dead_cvar);
    spin_unlock_irq_restore(&dead_lock, interrupts_enabled);
}