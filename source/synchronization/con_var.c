#include "con_var.h"
#include "../scheduler/scheduler.h"
#include "spin_lock.h"

void con_var_init(con_var* var) {
    var->wait_list = (linked_list) LINKED_LIST_STATIC_INITIALIZER;
}

void con_var_wait(con_var* var, lock* lock) {
    linked_list_add_last_node(&var->wait_list, get_current_thread_node());
    schedule_thread_block();
    spin_unlock(lock);
    schedule();

    return spin_lock(lock);
}

bool con_var_wait_irq_save(con_var* var, lock* lock, bool interrupts_enabled) {
    linked_list_add_last_node(&var->wait_list, get_current_thread_node());
    schedule_thread_block();
    spin_unlock_irq_restore(lock, interrupts_enabled);
    schedule();

    return spin_lock_irq_save(lock);
}

void con_var_signal(con_var* var) {
    if (var->wait_list.size != 0) {
        linked_list_node* waiter =
            linked_list_remove_first_node(&var->wait_list);
        schedule_thread(waiter);
    }
}

void con_var_broadcast(con_var* var) {
    while (var->wait_list.size != 0) {
        linked_list_node* waiter =
            linked_list_remove_first_node(&var->wait_list);
        schedule_thread(waiter);
    }
}