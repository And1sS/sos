#include "timer.h"
#include "../lib/types.h"
#include "../threading/scheduler.h"

static volatile u64 ticks = 0;

struct cpu_context* handle_timer_interrupt(struct cpu_context* context) {
    ticks++;

    schedule();

    return context;
}