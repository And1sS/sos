#include "../lib/types.h"
#ifndef SOS_ARCH_SIGNAL_H
#define SOS_ARCH_SIGNAL_H

struct cpu_context;

bool arch_is_userspace_context(struct cpu_context* context);

void arch_install_user_signal_handler(struct cpu_context* context,
                                 u64 signal_handler);

#endif // SOS_ARCH_SIGNAL_H
