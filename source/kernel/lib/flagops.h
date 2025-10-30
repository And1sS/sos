#ifndef SOS_FLAGOPS_H
#define SOS_FLAGOPS_H

#include "../synchronization/atomics.h"

#define ATOMIC_SET_FLAGS(flags, mask) atomic_or(&(flags), (mask))

#define ATOMIC_TEST_FLAG(flags, flag) (atomic_get(&(flags)) & flag)

#endif // SOS_FLAGOPS_H
