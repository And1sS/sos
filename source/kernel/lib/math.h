#ifndef SOS_MATH_H
#define SOS_MATH_H

#include "types.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

i8 msb_u32(u32 value);
i8 msb_u64(u64 value);

i8 lsb_u32(u32 value);
i8 lsb_u64(u64 value);

#endif // SOS_MATH_H
