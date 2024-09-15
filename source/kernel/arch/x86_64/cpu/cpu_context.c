#include "cpu_context.h"
#include "../../../lib/kprint.h"
#include "../../common/context.h"
#include "gdt.h"

bool arch_is_userspace_context(struct cpu_context* context) {
    cpu_context* arch_context = (cpu_context*) context;
    return arch_context->cs == USER_CODE_SEGMENT_SELECTOR;
}

void print_cpu_register(string reg, u64 value) {
    print(reg);
    print(" : ");
    print_u64_hex(value);
    println("");
}

void arch_print_cpu_context(struct cpu_context* context) {
    cpu_context* arch_context = (cpu_context*) context;

    println("---------------cpu context:---------------");
    print_cpu_register("ds", arch_context->ds);
    print_cpu_register("rax", arch_context->rax);
    print_cpu_register("rbx", arch_context->rbx);
    print_cpu_register("rcx", arch_context->rcx);
    print_cpu_register("rdx", arch_context->rdx);
    print_cpu_register("rsi", arch_context->rsi);
    print_cpu_register("rdi", arch_context->rdi);
    print_cpu_register("rbp", arch_context->rbp);
    print_cpu_register("r8", arch_context->r8);
    print_cpu_register("r9", arch_context->r9);
    print_cpu_register("r10", arch_context->r10);
    print_cpu_register("r11", arch_context->r11);
    print_cpu_register("r12", arch_context->r12);
    print_cpu_register("r13", arch_context->r13);
    print_cpu_register("r14", arch_context->r14);
    print_cpu_register("r15", arch_context->r15);
    print_cpu_register("rip", arch_context->rip);
    print_cpu_register("cs", arch_context->cs);
    print_cpu_register("rflags", arch_context->rflags);
    print_cpu_register("rsp", arch_context->rsp);
    print_cpu_register("ss", arch_context->ss);
}

void arch_set_syscall_return_value(struct cpu_context* context, u64 value) {
    ((cpu_context*) context)->rax = value;
}

u64 arch_get_instruction_pointer(struct cpu_context* context) {
    return ((cpu_context*) context)->rip;
}

void arch_clone_cpu_context(struct cpu_context* src, struct cpu_context* dst) {
    *((cpu_context*) dst) = *((cpu_context*) src);
}