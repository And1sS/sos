#ifndef SOS_ARCH_COMMON_CONTEXT_H
#define SOS_ARCH_COMMON_CONTEXT_H

#include "../../lib/types.h"

struct cpu_context;

bool arch_is_userspace_context(struct cpu_context* context);

void arch_set_syscall_return_value(struct cpu_context* context, u64 value);

u64 arch_get_instruction_pointer(struct cpu_context* context);

void arch_clone_cpu_context(struct cpu_context* src, struct cpu_context* dst);

void arch_print_cpu_context(struct cpu_context* context);

#endif // SOS_ARCH_COMMON_CONTEXT_H
