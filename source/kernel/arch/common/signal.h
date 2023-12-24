#ifndef SOS_ARCH_COMMON_SIGNAL_H
#define SOS_ARCH_COMMON_SIGNAL_H

#include "../../lib/types.h"
#include "../../signal/signal.h"

struct cpu_context;

void arch_install_user_signal_handler(struct cpu_context* context,
                                      signal_handler* handler);

#endif // SOS_ARCH_COMMON_SIGNAL_H