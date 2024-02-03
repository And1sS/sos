#ifndef SOS_CPUID_H
#define SOS_CPUID_H

#include "../../../lib/types.h"

#define CPUID_VENDOR 0x00000000
#define CPUID_FEATURES 0x00000001
#define CPUID_EXT_VENDOR 0x80000000
#define CPUID_EXT_FEATURES 0x80000001

#define CPUID_EXECUTE_DISABLE_FEATURE_OFFSET 20

void cpuid(u32 reg, u32* eax, u32* ebx, u32* ecx, u32* edx);

#endif // SOS_CPUID_H
