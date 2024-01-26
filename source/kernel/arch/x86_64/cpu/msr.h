#include "../../../lib/types.h"
#ifndef SOS_MSR_H
#define SOS_MSR_H

u64 msr_read(u32 reg);
void msr_write(u32 msr, u64 value);

#endif // SOS_MSR_H
