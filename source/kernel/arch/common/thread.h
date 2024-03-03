#ifndef SOS_ARCH_COMMON_THREAD_H
#define SOS_ARCH_COMMON_THREAD_H

#include "../../threading/thread/kthread.h"
#include "../../threading/thread/uthread.h"

struct cpu_context;

struct cpu_context* arch_uthread_context_init(uthread* thrd,
                                              uthread_func* func);

struct cpu_context* arch_kthread_context_init(kthread* thrd,
                                              kthread_func* func);

#endif // SOS_ARCH_COMMON_THREAD_H
