#include "con_var.h"
#include "../scheduler/scheduler.h"
#include "spin_lock.h"

void con_var_init(con_var* var) {
    var->wait_queue = (queue) QUEUE_STATIC_INITIALIZER;
}

void con_var_wait(con_var* var, lock* lock) {
    queue_push(&var->wait_queue, get_current_thread_node());
    schedule_thread_block();
    spin_unlock(lock);
    schedule();

    spin_lock(lock);
}

bool con_var_wait_irq_save(con_var* var, lock* lock, bool interrupts_enabled) {
    queue_push(&var->wait_queue, get_current_thread_node());
    schedule_thread_block();
    spin_unlock_irq_restore(lock, interrupts_enabled);
    schedule();

    return spin_lock_irq_save(lock);
}

void con_var_signal(con_var* var) {
    if (var->wait_queue.size != 0) {
        linked_list_node* waiter = queue_pop(&var->wait_queue);
        schedule_thread(waiter);
    }
}

void con_var_broadcast(con_var* var) {
    while (var->wait_queue.size != 0) {
        linked_list_node* waiter = queue_pop(&var->wait_queue);
        schedule_thread(waiter);
    }
}