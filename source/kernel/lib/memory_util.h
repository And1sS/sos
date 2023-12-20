#ifndef SOS_MEMORY_UTIL_H
#define SOS_MEMORY_UTIL_H

#include "types.h"

void* memset(void* dst, u8 val, u64 len);
void memcpy(void* dst, void* src, u64 len);

#endif // SOS_MEMORY_UTIL_H
