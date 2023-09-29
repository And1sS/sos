#include "alignment.h"

u64 align_to_lower(u64 value, u64 alignment) {
    if (value % alignment == 0)
        return value;
    return value - (value % alignment);
}

u64 align_to_upper(u64 value, u64 alignment) {
    if (value % alignment == 0)
        return value;
    return value + alignment - (value % alignment);
}