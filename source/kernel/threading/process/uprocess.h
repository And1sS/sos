#ifndef SOS_UPROCESS_H
#define SOS_UPROCESS_H

#include "process.h"

#define uprocess process

uprocess* uprocess_create();

u64 uprocess_fork(struct cpu_context* context);

#endif // SOS_UPROCESS_H
