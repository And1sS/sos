#include "../lib/types.h"
#ifndef SOS_THREADING_H
#define SOS_THREADING_H

void threading_init();

u64 threading_allocate_tid();
bool threading_free_tid(u64 tid);

#endif // SOS_THREADING_H
