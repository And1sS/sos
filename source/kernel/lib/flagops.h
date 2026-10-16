#ifndef SOS_FLAGOPS_H
#define SOS_FLAGOPS_H

#include "../synchronization/atomics.h"

#define SET_FLAGS(flags, mask) atomic_or(&(flags), (mask))

#define RESET_FLAGS(flags, mask) atomic_and(&(flags), ~(mask))

#define TEST_FLAG(flags, flag) ((atomic_get(&(flags)) & flag) != 0)

#endif // SOS_FLAGOPS_H
