#include "../lib/types.h"
#ifndef SOS_THREADING_H
#define SOS_THREADING_H

void threading_init();

bool threading_allocate_tid(u64* result);
bool threading_free_tid(u64 tid);

bool threading_allocate_pid(u64* result);
bool threading_free_pid(u64 pid);

#endif // SOS_THREADING_H
