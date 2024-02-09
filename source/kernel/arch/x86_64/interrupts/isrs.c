#include "isrs.h"
#include "../../../lib/kprint.h"
#include "../../../synchronization/rw_spin_lock.h"
#include "pic.h"

static rw_spin_lock irq_handlers_lock = RW_LOCK_STATIC_INITIALIZER;
static irq_handler* irq_handlers[256] = {0};

static rw_spin_lock exception_handlers_lock = RW_LOCK_STATIC_INITIALIZER;
static exception_handler* exception_handlers[32] = {0};

static struct cpu_context* handle_unknown_irq(u8 irq_num,
                                              struct cpu_context* context) {

    print("Irq #");
    print_u64(irq_num);
    println("");
    return context;
}

static struct cpu_context* handle_irq(u8 irq_num, struct cpu_context* context) {
    rw_spin_lock_read_irq(&irq_handlers_lock);
    irq_handler* handler = irq_handlers[irq_num];
    rw_spin_unlock_read_irq(&irq_handlers_lock);

    struct cpu_context* new_context =
        handler != NULL ? handler(context)
                        : handle_unknown_irq(irq_num, context);

    if (irq_num >= 32 && irq_num < 47) {
        pic_ack(irq_num);
    }

    return new_context;
}

struct cpu_context* handle_soft_irq(u8 irq_num, struct cpu_context* context) {
    return handle_irq(irq_num, context);
}

struct cpu_context* handle_hard_irq(u8 irq_num, struct cpu_context* context) {
    return handle_irq(irq_num, context);
}

static struct cpu_context* handle_unknown_exception(
    u8 exception_num, u64 error_code, struct cpu_context* context) {

    print("Exception #");
    print_u64(exception_num);
    print(", error code: ");
    print_u64_hex(error_code);
    println("");
    return context;
}

struct cpu_context* handle_exception(u8 exception_num, u64 error_code,
                                     struct cpu_context* context) {

    rw_spin_lock_read_irq(&exception_handlers_lock);
    exception_handler* handler = exception_handlers[exception_num];
    rw_spin_unlock_read_irq(&exception_handlers_lock);

    struct cpu_context* new_context =
        handler != NULL
            ? handler(context)
            : handle_unknown_exception(exception_num, error_code, context);

    return new_context;
}

void mount_irq_handler(u8 irq_num, irq_handler* handler) {
    rw_spin_lock_write_irq(&irq_handlers_lock);
    irq_handlers[irq_num] = handler;
    rw_spin_unlock_write_irq(&irq_handlers_lock);
}

void mount_exception_handler(u8 exception_num, exception_handler* handler) {
    rw_spin_lock_write_irq(&exception_handlers_lock);
    exception_handlers[exception_num] = handler;
    rw_spin_unlock_write_irq(&exception_handlers_lock);
}