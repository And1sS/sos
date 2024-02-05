#include "../../../interrupts/irq.h"
#include "../../../lib/kprint.h"
#include "../../../scheduler/scheduler.h"
#include "../../common/signal.h"
#include "../cpu/cpu_context.h"
#include "../cpu/io.h"
#include "../cpu/registers.h"
#include "idt.h"

const u8 OCW2_EOI_OFFSET = 5;
volatile u64 ticks = 0;

struct cpu_context* handle_interrupt(u8 interrupt_number, u64 error_code,
                                     struct cpu_context* context) {

    if (interrupt_number == 32) {
        ticks++;
        return context_switch(context);
    } else if (interrupt_number == 250) {
        return context_switch(context);
    } else if (interrupt_number == 13) {
        print("PROTECTION FAULT Error code: ");
        print_u64_hex(error_code);
        panic("");
    } else if (interrupt_number == 14) {
        print("PAGE FAULT Error code: ");
        print_u64_hex(error_code);
        print(" Faulty addr(CR2): ");
        u64 cr2 = get_cr2();
        print_u64_hex(cr2);
        println("");

        // Just for test
        // TODO: Refactor this
        thread* current = get_current_thread();
        if (current && current->kernel_thread) {
            panic("");
        } else {
            thread_signal(get_current_thread(), SIGSEGV);
        }
    } else {
        print("isr #");
        print_u64(interrupt_number);
        print(", error code: ");
        print_u64_hex(error_code);
        println("");
    }

    return context;
}

struct cpu_context* handle_hardware_interrupt(u8 interrupt_number,
                                              struct cpu_context* context) {

    struct cpu_context* new_context =
        handle_interrupt(interrupt_number, 0, context);

    outb(MASTER_PIC_COMMAND_ADDR, 1 << OCW2_EOI_OFFSET);
    io_wait();

    // slave PIC interrupt
    if (interrupt_number >= 40 && interrupt_number < 48) {
        outb(SLAVE_PIC_COMMAND_ADDR, 1 << OCW2_EOI_OFFSET);
        io_wait();
    }

    return new_context;
}

struct cpu_context* handle_software_interrupt(u8 interrupt_number,
                                              u64 error_code,
                                              struct cpu_context* context) {

    return handle_interrupt(interrupt_number, error_code, context);
}

struct cpu_context* handle_syscall(u64 arg0, u64 arg1, u64 arg2, u64 arg3,
                                   u64 arg4, u64 arg5, u64 syscall_number,
                                   struct cpu_context* context) {

    // no checks for now, because this is just for test
    if (syscall_number == 0) { // print
        print((string) arg0);
        ((cpu_context*) context)->rax = 0;
    } else if (syscall_number == 1) { // signal handler installation
        thread* current = get_current_thread();
        if (current) {
            bool set = thread_set_sigaction(current, arg0, *(sigaction*) arg1);
            ((cpu_context*) context)->rax = set ? 0 : -1;
        }
    } else if (syscall_number == 2) {
        thread_exit(arg0);
    } else if (syscall_number == 4) { // signal return
        arch_return_from_signal_handler(context);
    } else if (syscall_number == 5) {
        print_u64(arg0);
    } else {
        print("syscall num: ");
        print_u64(syscall_number);
        print(" arg0: ");
        print_u64(arg0);
        print(" arg1: ");
        print_u64(arg1);
        print(" arg2: ");
        print_u64(arg2);
        print(" arg3: ");
        print_u64(arg3);
        print(" arg4: ");
        print_u64(arg4);
        print(" arg5: ");
        print_u64(arg5);
        println("");
    }

    return context;
}