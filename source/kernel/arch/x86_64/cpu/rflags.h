#ifndef SOS_RFLAGS_H
#define SOS_RFLAGS_H

#include "../../../lib/types.h"

#define RFLAGS_IRQ_ENABLED_OFFSET 9

#define RFLAGS_INIT_FLAGS 0x2
#define RFLAGS_IRQ_ENABLED_FLAG (1 << RFLAGS_IRQ_ENABLED_OFFSET)

u64 get_rflags();

#endif // SOS_RFLAGS_H
