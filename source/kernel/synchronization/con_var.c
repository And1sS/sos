#include "con_var.h"
#include "../threading/scheduler.h"
#include "barriers.h"
#include "spin_lock.h"

void con_var_init(con_var* var) {
    var->wait_queue = (queue) QUEUE_STATIC_INITIALIZER;
}

void con_var_wait(con_var* var, lock* lock) {
    thread* current = get_current_thread();
    queue_node waiter_node = QUEUE_NODE_OF(current);
    queue_push(&var->wait_queue, &waiter_node);
    current->state = BLOCKED; // no need to guard state with barriers, since
                              // next time it will be used by scheduler it will
                              // be on same core and all modern architectures
                              // preserve memory order for data dependencies,
                              // and other cores will see this value carried
                              // through scheduler lock

    spin_unlock(lock);
    schedule();

    spin_lock(lock);
    // Try to remove node from con_var wait queue if we were woken up by a
    // reason other that signalling current con_var
    queue_remove(&var->wait_queue, &waiter_node);
}

bool con_var_wait_irq_save(con_var* var, lock* lock, bool interrupts_enabled) {
    thread* current = get_current_thread();
    queue_node waiter_node = QUEUE_NODE_OF(current);
    queue_push(&var->wait_queue, &waiter_node);
    current->state = BLOCKED; // no need to guard state with barriers, since
                              // next time it will be used by scheduler it will
                              // be on same core and all modern architectures
                              // preserve memory order for data dependencies,
                              // and other cores will see this value carried
                              // through scheduler lock

    spin_unlock_irq_restore(lock, interrupts_enabled);
    schedule();

    interrupts_enabled = spin_lock_irq_save(lock);
    // Try to remove node from con_var wait queue if we were woken up by a
    // reason other that signalling current con_var
    queue_remove(&var->wait_queue, &waiter_node);
    return interrupts_enabled;
}

void con_var_signal(con_var* var) {
    if (var->wait_queue.size != 0) {
        queue_node* waiter = queue_pop(&var->wait_queue);
        schedule_thread((thread*) waiter->value);
    }
}

void con_var_broadcast(con_var* var) {
    while (var->wait_queue.size != 0) {
        queue_node* waiter = queue_pop(&var->wait_queue);
        schedule_thread((thread*) waiter->value);
    }
}