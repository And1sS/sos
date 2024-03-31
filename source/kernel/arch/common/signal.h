#ifndef SOS_ARCH_COMMON_SIGNAL_H
#define SOS_ARCH_COMMON_SIGNAL_H

#include "../../lib/types.h"
#include "../../signal/signal.h"

struct cpu_context;

void arch_enter_signal_handler(struct cpu_context* context,
                               signal_handler* handler);

// returns value of register, that is used for carrying syscall return value and
// will be scratched, to preserve it.
u64 arch_return_from_signal_handler(struct cpu_context* context);

#endif // SOS_ARCH_COMMON_SIGNAL_H