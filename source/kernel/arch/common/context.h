#ifndef SOS_ARCH_COMMON_CONTEXT_H
#define SOS_ARCH_COMMON_CONTEXT_H

#include "../../lib/types.h"

struct cpu_context;

bool arch_is_userspace_context(struct cpu_context* context);

#endif // SOS_ARCH_COMMON_CONTEXT_H
